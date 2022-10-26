#include "TCPChecker.h"
#include "Network/Utils.h"
#include <Log/Log.h>
#include <Network/TCPClient.h>
#include <Converter/Converter.h>
#include "ShadowSocksMain.h"
#include "ShadowSocksController.h"

using __DP_LIB_NAMESPACE__::parse;
using __DP_LIB_NAMESPACE__::UInt;

namespace TCPChecker {
	void TCPCheckerWorker::Check() {
		DP_LOG_DEBUG << "Start check server " << host << ":" << port << "\n";
		if (bootstrapDNS.size() > 4) {
			__DP_LIB_NAMESPACE__::global_config.setDNS(bootstrapDNS);
		}
		if (checkingMode == ServerCheckingMode::Off || __DP_LIB_NAMESPACE__::TCPClient::IsCanConnect(host, port)) {
			if (this->task == nullptr || checkingMode == ServerCheckingMode::Off ) {
				DP_LOG_DEBUG << "Server " << host << ":" << port << " is Allow";
				callback(TCPStatus::Allow, this);
			} else {
				auto & ctrl = ShadowSocksController::Get();
				auto flags = ctrl.makeCheckStruct();
				flags.one_loop = true;
				flags.auto_check_mode = checkingMode == ServerCheckingMode::DeepCheck ? AutoCheckingMode::Ip : AutoCheckingMode::Work;
				ctrl.check_server(this->srv, this->task, flags);
				if (ctrl.getConfig().auto_check_mode !=  AutoCheckingMode::Off)
					ctrl.SaveCashe();
				if (srv->check_result.isRun) {
					DP_LOG_DEBUG << "Deep check Server " << host << ":" << port << " is Allow";
					callback(TCPStatus::Allow, this);
				} else {
					DP_LOG_DEBUG << "Deep check Server " << host << ":" << port << " is Down";
					callback(TCPStatus::Disallow, this);
				}
			}
		} else {
			DP_LOG_DEBUG << "Server " << host << ":" << port << " is Down";
			callback(TCPStatus::Disallow, this);
		}
	}
	void TCPCheckerWorker::CheckInThread() {
		thread = new __DP_LIB_NAMESPACE__::Thread(&TCPCheckerWorker::Check, this);
		thread->SetName("TCPCheckerWorker::Check");
		thread->start();
	}

	List<String> resolveIpAddr(const String & host, const String & bootstrapDNS, bool IGNORECHECKSERVER) {
		List<String> ip_addrs;
		if (__DP_LIB_NAMESPACE__::isIPv4(host)) {
			ip_addrs.push_back(host);
		} else {
			if (bootstrapDNS.size() > 4) {
				__DP_LIB_NAMESPACE__::global_config.setDNS(bootstrapDNS);
			}
			ip_addrs = __DP_LIB_NAMESPACE__::resolveDomainList(host);
			if (ip_addrs.size() == 0) {
				if (IGNORECHECKSERVER)
					ip_addrs.push_back(host);
				else
					DP_LOG_ERROR << "Can't resolve host " <<  host << "\n";
			}
		}
		return ip_addrs;
	}

	_Server * TCPCheckerLoop::Check(_Task * task, List<_Server*> & servers, const String & bootstrapDNS, ServerCheckingMode checkingMode, std::function<String(String)> replaceVariables) {
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

			if (!isNumber(port)) {
				DP_LOG_ERROR << "Invalid port on server " << srv->name;
				continue;
			}

			__DP_LIB_NAMESPACE__::List<String> ip_addrs = resolveIpAddr(host, bootstrapDNS, checkingMode == ServerCheckingMode::Off);
			if (ip_addrs.size() == 0)
				continue;

			for (const String & ip : ip_addrs) {
				TCPCheckerWorker * worker = new TCPCheckerWorker(
										task,
										srv,
										ip,
										parse<UInt>(port),
										callback,
										bootstrapDNS,
										checkingMode
									  );
				if (task != nullptr || ( srv->check_result.isRun && srv->check_result.ipAddr.size() > 0 ) )
					good_workers.push_back(worker);
				else
					bad_workers.push_back(worker);
			}
		}
		if (good_workers.size() == 0 && bad_workers.size()==0) {
			delete lock_status;
			return result;
		}
		if (good_workers.size() > 0) {
			lock_status->lock();
			active_workers = good_workers.size();
			for (TCPCheckerWorker * worker : good_workers)
				worker->CheckInThread();
			lock_status->lock();
		}
		delete lock_status;

		if (result == nullptr && bad_workers.size() > 0) {
			lock_status = new std::mutex();
			lock_status->lock();
			active_workers = bad_workers.size();
			for (TCPCheckerWorker * worker : bad_workers)
				worker->CheckInThread();
			lock_status->lock();
			delete lock_status;
		}
		return result;
	}

	List<_Server*> TCPCheckerLoop::CheckAll(_Task * task, List<_Server*> & servers, const String & bootstrapDNS, ServerCheckingMode checkingMode, std::function<String(String)> replaceVariables) {
		Join();

		std::mutex * lock_status = new std::mutex();
		List<_Server*> result;

		std::function<void (TCPStatus, TCPCheckerWorker *)> callback = [this, &lock_status, &result, replaceVariables](TCPStatus stat, TCPCheckerWorker * worker) {
			this->lopperStatus.lock();
			this->active_workers --;

			if (stat == TCPStatus::Disallow || stat == TCPStatus::None) {
				if (this->active_workers == 0)
					lock_status->unlock();
				this->lopperStatus.unlock();
				return;
			}
			_Server * s = worker->getServer()->Copy(replaceVariables);
			s->host = worker->getHost();
			result.push_back(s);
			if (this->active_workers == 0)
				lock_status->unlock();
			this->lopperStatus.unlock();
		};


		for (_Server * srv : servers) {
			String port = replaceVariables(srv->port);
			String host = replaceVariables(srv->host);

			if (!isNumber(port)) {
				DP_LOG_ERROR << "Invalid port on server " << srv->name;
				continue;
			}

			__DP_LIB_NAMESPACE__::List<String> ip_addrs = resolveIpAddr(host, bootstrapDNS, checkingMode == ServerCheckingMode::Off);
			if (ip_addrs.size() == 0)
				continue;

			for (const String & ip : ip_addrs) {
				TCPCheckerWorker * worker = new TCPCheckerWorker(
										task,
										srv,
										ip,
										parse<UInt>(port),
										callback,
										bootstrapDNS,
										checkingMode
									  );
				if (task != nullptr || ( srv->check_result.isRun && srv->check_result.ipAddr.size() > 0 ) )
					good_workers.push_back(worker);
				else
					bad_workers.push_back(worker);
			}
		}
		if (good_workers.size() == 0 && bad_workers.size()==0) {
			delete lock_status;
			return result;
		}
		if (good_workers.size() > 0) {
			lock_status->lock();
			active_workers = good_workers.size();
			for (TCPCheckerWorker * worker : good_workers)
				worker->CheckInThread();
			lock_status->lock();
		}
		delete lock_status;

		if (result.size() == 0 && bad_workers.size() > 0) {
			lock_status = new std::mutex();
			lock_status->lock();
			active_workers = bad_workers.size();
			for (TCPCheckerWorker * worker : bad_workers)
				worker->CheckInThread();
			lock_status->lock();
			delete lock_status;
		}
		return result;
	}
}
