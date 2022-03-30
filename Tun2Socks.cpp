#include "Tun2Socks.h"
#include <Converter/Converter.h>
#include <chrono>
#include <Parser/SmartParser.h>
#include <mutex>
#include <algorithm>
#include <Log/Log.h>

using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::IStrStream;
using __DP_LIB_NAMESPACE__::trim;
using __DP_LIB_NAMESPACE__::startWithN;
using __DP_LIB_NAMESPACE__::SmartParser;

String Tun2Socks::appPath;
String Tun2Socks::tun2socksPath;

void Tun2Socks::WaitForStart() {
	if (run != nullptr)
		run->WaitForStart();
	if (tun2socks != nullptr)
		tun2socks->WaitForStart();
}

#ifdef DP_WIN
	char asciitolower(char in) {
		if (in <= 'Z' && in >= 'A')
			return in - ('Z' - 'z');
		return in;
	}

	String Tun2Socks::DetectTunAdapterIF(const String & name) {
		Application app("C:\\Windows\\System32\\ipconfig.exe");
		app << "/all";
		IStrStream in;
		in.str(app.ExecAll());
		// 00 ff fb fd 87 48
		String mac="";
		bool finded = false;
		String EthName = " " + name + ":";

		// Нужно найти MAC сетевого интерфейса:
		/*
Ethernet adapter tuntap:

   Состояние среды. . . . . . . . : Среда передачи недоступна.
   DNS-суффикс подключения . . . . . :
   Описание. . . . . . . . . . . . . : TAP-Windows Adapter V9
   Физический адрес. . . . . . . . . : 00-FF-FB-FD-87-48
   DHCP включен. . . . . . . . . . . : Да
   Автонастройка включена. . . . . . : Да
		*/
		while (!in.eof()) {
			String line = "";
			getline(in, line);
			auto pos = line.find(EthName);
			if (pos != line.npos) {
				String oo = line.substr(0, pos);
				// Нашли
				// Описание. . . . . . . . . . . . . : TAP-Windows Adapter V9
				// Перед название сетевого интерфейса нет точек.
				if (oo.find(".") != oo.npos)
					continue;
				finded = true;
			}
			if (finded) {
				// Если ранее нашли описание сетевого интерфейса, то ищем MAC адрес
				SmartParser parser("*: ${1}-${2}-${3}-${4}-${5}-${6}");
				if (parser.Check(line)) {
					for (int i = 1; i <= 6; i++) {
						String d = parser.Get(toString(i));
						std::transform(d.begin(), d.end(), d.begin(), asciitolower);
						if (i != 1)
							mac += " ";
						mac += d;
					}
					mac = trim(mac);
					break;
				}
			}
		}
		if (mac.size() < 3)
			return "";

		Application p2 ("C:\\Windows\\System32\\ROUTE.EXE");
		p2 << "print";
		in.str(p2.ExecAll());
		while (!in.eof()) {
			String line = "";
			getline(in, line);
			auto pos = line.find(mac);
			// 15...00 ff fb fd 87 48 ......TAP-Windows Adapter V9
			if (pos != line.npos) {
				pos = line.find(".");
				String res = line.substr(0, pos);
				res =trim(res);
				return res;
			}
		}
		return "";
	}

	void Tun2Socks::WaitForIFInited(const String & name){
		while (true) {
			Application app("C:\\Windows\\System32\\ipconfig.exe");
			app << "/all";
			IStrStream in;
			in.str(app.ExecAll());
			bool finded = false;
			String EthName = " " + name + ":";

			while (!in.eof()) {
				String line = "";
				getline(in, line);
				auto pos = line.find(EthName);
				if (pos != line.npos) {
					String oo = line.substr(0, pos);
					// Нашли
					// Описание. . . . . . . . . . . . . : TAP-Windows Adapter V9
					// Перед название сетевого интерфейса нет точек.
					if (oo.find(".") != oo.npos)
						continue;
					finded = true;
				}
				if (finded) {
					// Если ранее нашли описание сетевого интерфейса, то ищем MAC адрес
					SmartParser parser("IPv4-*: ${1}.${2}.${3}.${4}*");
					if (parser.Check(line))
						return;
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	String Tun2Socks::DetectDefaultRoute() {
		Application p ("C:\\Windows\\System32\\ROUTE.EXE");
		p << "print";
		IStrStream in;
		in.str(p.ExecAll());
		int mode = 0;
		while (!in.eof()) {
			String line;
			getline(in, line);
			// Ищем, где начинается таблица маршрутизации
			if (mode == 0) {
				if (__DP_LIB_NAMESPACE__::startWithN(line, "IPv4"))
					mode = 1;
				continue;
			}
			line = trim(line);

			auto all = LiteralReader::readAllLiterals(line);
			if (all.size() != 5)
				continue;
			// Пропускаем заголовок
//			if (mode == 1) {
//				mode = 2;
//				continue;
//			}
			if ((*all.begin()->begin()) == "0.0.0.0") {
				auto it = all.begin();
				for (int i = 0; i < 2; i++)
					it++;
				return *(it->begin());
			}
		}
		return "";
	}
	String Tun2Socks::DetectInterfaceName() {
		Application app("C:\\Windows\\System32\\ipconfig.exe");
		app << "/all";
		IStrStream in;
		in.str(app.ExecAll());
		String prevInterfaceName = "";
		const String TAP_DRIVER_NAME="TAP-Windows Adapter V9";
		while (!in.eof()) {
			String line = "";
			getline(in, line);

			line = trim(line);
			// С большой вероятностью это наименовение интерфейса
			if (__DP_LIB_NAMESPACE__::endWithN(line, ":") && !__DP_LIB_NAMESPACE__::endWithN(line, ". :")) {
				auto all = LiteralReader::readAllLiterals(line);
				if (all.size() >= 1) {
					auto it = all.rbegin();
					prevInterfaceName = *(it->begin());
				}
			}
			auto pos = line.find(TAP_DRIVER_NAME);
			if (pos != line.npos) {
				return prevInterfaceName;
			}
		}

	return "";
}

#endif

#ifdef DP_LIN
	String Tun2Socks::DetectInterfaceName() {
		// В Linux нечего искать.
		return "TunTap";
	}
	void getCurrentDefaultIface(String & name, String & gw) {
		DP_LOG_INFO << "try detect defaul iface\n";
		Application p ("/sbin/route");
		p << "-n";
		String all = p.ExecAll();
		IStrStream str;
		str.str(all);
		int i = 0;
		String _gw = "";
		String _ifname = "";
		while (!str.eof()) {
			String line = "";
			getline(str, line);
			DP_LOG_DEBUG << line << "\n";
			auto all = LiteralReader::readAllLiterals(line);
			int status = 0;
			int i = 0;
			//Destination	 Gateway		 Genmask		 Flags Metric Ref	Use Iface
			//0.0.0.0		 10.17.100.1	 0.0.0.0		 UG	20100  0		0 enp0s3
			for (const List<String> & t : all) {
				String val = *t.begin();
				if (i == 0) {
					if (val[0] == 'D' || val[0] == 'K') {
						status = 1;
						break;
					}
					if (val != "0.0.0.0") {
						status = 1;
						break;
					}
				}
				if (i == 1)
					_gw = val;
				if (i == 7)
					_ifname = val;
				if (_gw.size() > 0 && _ifname.size() > 0)
					break;

				i++;

			}
			if (status == 1)
				continue;

		}
		DP_LOG_INFO << "Detected interface " << _ifname << " gw = " << _gw << "\n";
		name = _ifname;
		gw = _gw;
	}

	String Tun2Socks::DetectDefaultRoute() {
		String name, gateway;
		getCurrentDefaultIface(name, gateway);
		return gateway;
	}
#endif

List<char> LiteralReader::delimers{'[', ',', ']', '=', '>', '<', ':', ';', '+'/*, '-'*/, ' ', '\n', '\r' };

void Tun2Socks::Start(std::function<void()> _onSuccess, OnShadowSocksError _onCrash) {
	if (this->config.dns.size() < 1) {
		throw EXCEPTION("Need one or more dns servers");
		return;
	}
	#ifdef DP_LIN
		getCurrentDefaultIface(this->iface_name, this->iface_gw);
	#endif
	run = new Application(appPath);
	Application & ap = *run;
	{
		String tmtp = proxyServer + ":";
		tmtp = tmtp + toString(proxyPort);
		ap << "-proxyServer" << tmtp;
	}
	ap << "-proxyType" << "socks";
	ap << "-tunAddr" << "192.168.198.2";
	ap << "-tunDns";
	ap.SetReadedFunc(ReadFuncProc);
	if (!config.isDNS2Socks){
		String dns = "";
		for (const String & dn : this->config.dns)
			dns = dns + dn + ",";
		dns = dns.substr(0, dns.size()-1);
		ap << dns;
	} else {
		ap << "127.0.0.1";
	}
	ap << "-tunGw" << "192.168.198.1";
	ap << "-tunMask" << "255.255.255.0";
	ap << "-tunName" << this->config.tunName;
	ap << "-udpTimeout" << this->udpTimeout;
	ap << "-loglevel" << "none";

	ap.SetOnCloseFunc([this, _onCrash]() {
		ExitStatus status;
		status.code = ExitStatusCode::ApplicationError;
		status.str = "Tun2Socks unexpected exited";
		_onCrash(config.name, status);
	});


	#ifdef DP_WIN
		int * state = new int(0);

		ap.SetReadedFunc([state] (const String & str) {
			if (state == nullptr)
				return;
			if (*state == -99090) {
				return;
			}

			if (str.find("failed ") != str.npos)
				*state = -60;

			if (str.find("through DHCP") != str.npos)
				*state = 60;
		});

		ap.ExecInNewThread();
		ap.WaitForStart();

		while ((*state) == 0)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		if ((*state) == -60) {
			return;
		}
		if ((*state) == 60)
			*state = -99090;

		String IF = DetectTunAdapterIF(this->config.tunName);
		if (IF.size() == 0) {
			DP_LOG_FATAL << "Fail to Detect interface id\n";
			return;
		}
	#else
		ap.ExecInNewThread();
		ap.WaitForStart();
		if (ap.isFinished()) {
			DP_LOG_FATAL << "Tun2Socks crashed";
			ExitStatus status;
			status.code = ExitStatusCode::ApplicationError;
			status.str = "Tun2Socks unexpected exited";
			_onCrash(config.name, status);
			return;
		}
		// Ждем, пока поднимится интерфейс
		while (true) {
			Application p ("/sbin/ifconfig");
			p << "-a";
			IStrStream in;
			in.str(p.ExecAll());
			bool isReady = false;
			while (!in.eof()) {
				String line ;
				getline(in, line);
				if (startWithN(line, this->config.tunName)) {
					DP_LOG_DEBUG << "Tun interface ready.";
					isReady = true;
					break;
				}
			}
			if (isReady)
				break;
			DP_LOG_DEBUG << "Tun interface is not ready. Wait.";
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		Application p ("/sbin/ifconfig");
		//ifconfig tap 192.168.198.2 netmask 255.255.255.0
		p << this->config.tunName << "192.168.198.2" << "netmask" << "255.255.255.0";
		int * state = new int(0);
		p.SetOnCloseFunc([state]() {
			*state = 1;
		});
		p.Exec();
		p.WaitForStart();
		while ((*state) == 0)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		delete state;

	#endif

	{
		for (const String & ip : this->config.ignoreIP){
			#ifdef DP_WIN
				Application p ("C:\\Windows\\System32\\ROUTE.EXE");
				p << "-p" << "ADD" << ip << "MASK" << "255.255.255.255" << this->config.defaultRoute;
			#else
				//route add 92.119.161.225/32 dev tap
				Application p ("/sbin/route");
				p << "add" << (ip + "/32") << "gw" << this->iface_gw;
				p << "dev" << this->iface_name;
			#endif
			p.Exec();

		}
	}
	{
		#ifdef DP_WIN
			Application p ("C:\\Windows\\System32\\ROUTE.EXE");
			p << "-p" << "ADD" << "0.0.0.0" << "MASK" << "0.0.0.0" << "192.168.198.1" << "IF" << IF;
		#else
			Application p ("/sbin/route");
			p << "add" << "default" << "gw" << "192.168.198.1" << this->config.tunName;
		#endif
		p.Exec();
	}
	if (config.removeDefaultRoute) {
		deleteDefault = new __DP_LIB_NAMESPACE__::Thread(&Tun2Socks::ThreadLoop, this);
		deleteDefault->SetName("Tun2Socks::ThreadLoop");
		deleteDefault->start();
	}
	if (config.isDNS2Socks) {
		tun2socks = new Application(tun2socksPath);
		Application & t = *tun2socks;
		if (config.hideDNS2Socks)
			t << "/q";
		String tmtp = proxyServer + ":";
		tmtp = tmtp + toString(proxyPort);
		t << tmtp << (*this->config.dns.begin()) << "127.0.0.1:53";
		t.SetOnCloseFunc([this, _onCrash]() {
			ExitStatus status;
			status.code = ExitStatusCode::ApplicationError;
			status.str = "DNS2Socks unexpected exited";
			_onCrash(config.name, status);
		});
		t.ExecInNewThread();
		t.WaitForStart();
	}
	if ((!tun2socks->isFinished()) && (!ap.isFinished()))
		_onSuccess();

}

void Tun2Socks::DisableCrashFunc(){
	if (tun2socks != nullptr)
		tun2socks->SetOnCloseFunc(nullptr);
	if (run != nullptr)
		run->SetOnCloseFunc(nullptr);
}

void Tun2Socks::ThreadLoop() {
	while (!_is_exit) {
		#ifdef DP_WIN
			Application p ("C:\\Windows\\System32\\ROUTE.EXE");
			p << "delete" << "0.0.0.0" << "MASK" << "0.0.0.0" << this->config.defaultRoute;
		#else
			Application p ("/sbin/route");
			p << "del" << "default" << "gw" << this->iface_gw;
		#endif
		p.Exec();
		std::this_thread::sleep_for(std::chrono::milliseconds(sleepMS));
	}
}

void Tun2Socks::Stop() {
	_is_exit = true;
	for (const String & ip : this->config.ignoreIP){
		#ifdef DP_WIN
			Application p ("C:\\Windows\\System32\\ROUTE.EXE");
			p << "-p" << "DELETE" << ip << "MASK" << "255.255.255.255" << this->config.defaultRoute;
		#else
			Application p ("/sbin/route");
			p << "del" << ip << this->iface_name;
		#endif
		p.Exec();

	}
	{
		#ifdef DP_WIN
			Application p ("C:\\Windows\\System32\\ROUTE.EXE");
			p << "-p" << "DELETE" << "0.0.0.0" << "MASK" << "0.0.0.0" << "192.168.198.1";
		#else
			Application p ("/sbin/route");
			p << "del" << "default" << "gw" << this->config.tunName;
		#endif
		p.Exec();
	}
	if (config.removeDefaultRoute) {
		#ifdef DP_WIN
			Application p ("C:\\Windows\\System32\\ROUTE.EXE");
			p << "ADD" << "0.0.0.0" << "MASK" << "0.0.0.0" << this->config.defaultRoute;
		#else
			Application p ("/sbin/route");
			p << "add" << "default" << "gw" << this->iface_gw << this->iface_name;
		#endif
		p.Exec();
	}
	if (run == nullptr)
		return;
	run->KillNoJoin();
	run = nullptr;
	if (deleteDefault != nullptr) {
		deleteDefault->join();
		delete deleteDefault;
		deleteDefault = nullptr;
	}
	if (tun2socks != nullptr) {
		tun2socks->KillNoJoin();
		tun2socks = nullptr;
	}
}
