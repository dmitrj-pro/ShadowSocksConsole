#include "TCPChecker.h"
#include <Log/Log.h>
#include "TCPClient.h"
#include <Converter/Converter.h>
#include "ShadowSocksMain.h"
#include "libSimpleDNS/dns.h"

using __DP_LIB_NAMESPACE__::parse;
using __DP_LIB_NAMESPACE__::UInt;

namespace TCPChecker {
	void TCPCheckerWorker::Check() {
		__DP_LIB_NAMESPACE__::log << "Start check server " << host << ":" << port << "\n";
		if (IGNORECHECKSERVER || TCPClient::IsCanConnect(host, port, bootstrapDNS)) {
			__DP_LIB_NAMESPACE__::log << "Check server " << host << ":" << port << ":";
			__DP_LIB_NAMESPACE__::log << "Success" << "\n";
			callback(TCPStatus::Allow, this);
		} else
			callback(TCPStatus::Disallow, this);
	}
	void TCPCheckerWorker::CheckInThread() {
		thread = new std::thread(&TCPCheckerWorker::Check, this);
	}

	List<String> resolveIpAddr(const String & host, const String & bootstrapDNS, bool IGNORECHECKSERVER) {
		List<String> ip_addrs;
		if (isIPv4(host)) {
			ip_addrs.push_back(host);
		} else {
			ip_addrs = resolveDNS(host, bootstrapDNS);
			if (ip_addrs.size() == 0) {
				if (IGNORECHECKSERVER)
					ip_addrs.push_back(host);
				else
					__DP_LIB_NAMESPACE__::log << "Can't resolve host " <<  host << "\n";
			}
		}
		return ip_addrs;
	}

	_Server * TCPCheckerLoop::Check(List<_Server*> & servers, const String & bootstrapDNS, bool IGNORECHECKSERVER, std::function<String(String)> replaceVariables) {
		Join();

		std::mutex * lock_status = new std::mutex();
		_Server * result = nullptr;
		this->findet = false;

		std::function<void (TCPStatus, TCPCheckerWorker *)> callback = [this, &lock_status, &result, replaceVariables](TCPStatus stat, TCPCheckerWorker * worker) {
			this->lopperStatus.lock();
			this->active_workers --;

			if (this->findet) {
				this->lopperStatus.unlock();
				return;
			}

			if (stat == TCPStatus::Disallow || stat == TCPStatus::None) {
				if (this->active_workers == 0)
					lock_status->unlock();
				this->lopperStatus.unlock();
				return;
			}
			result = worker->getServer()->Copy(replaceVariables);
			result->host = worker->getHost();
			this->findet = true;
			lock_status->unlock();
			this->lopperStatus.unlock();
		};


		for (_Server * srv : servers) {
			String port = replaceVariables(srv->port);
			String host = replaceVariables(srv->host);

			if (!isNumber(port))
				throw EXCEPTION("Invalid port on server " + srv->name);

			__DP_LIB_NAMESPACE__::List<String> ip_addrs = resolveIpAddr(host, bootstrapDNS, IGNORECHECKSERVER);
			if (ip_addrs.size() == 0)
				continue;

			for (const String & ip : ip_addrs) {
				TCPCheckerWorker * worker = new TCPCheckerWorker(
										srv,
										ip,
										parse<UInt>(port),
										callback,
										bootstrapDNS,
										IGNORECHECKSERVER
									  );
				workers.push_back(worker);
			}
		}
		if (workers.size() == 0) {
			delete lock_status;
			return result;
		}
		lock_status->lock();
		active_workers = workers.size();
		for (TCPCheckerWorker * worker : workers)
			worker->CheckInThread();
		lock_status->lock();
		delete lock_status;
		return result;
	}
}
