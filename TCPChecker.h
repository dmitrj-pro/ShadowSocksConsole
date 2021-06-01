#pragma once

#include <DPLib.conf.h>
#include <functional>
#include <thread>
#include <mutex>

using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::List;
using __DP_LIB_NAMESPACE__::UInt;

struct _Server;

namespace TCPChecker {
	enum class TCPStatus { None, Allow, Disallow};

	class TCPCheckerWorker{
		private:
			std::function<void (TCPStatus, TCPCheckerWorker * )> callback;
			UInt port;
			String host;
			bool IGNORECHECKSERVER;
			String bootstrapDNS;
			_Server* srv;
			std::thread * thread = nullptr;
		public:
			TCPCheckerWorker(_Server* srv,
							const String & host,
							UInt port,
							 std::function<void (TCPStatus, TCPCheckerWorker * )> callback,
							 const String & bootstrapDNS,
							 bool IGNORECHECKSERVER = false)
				:
					srv(srv),
					bootstrapDNS(bootstrapDNS),
					callback(callback),
					host(host),
					port(port),
					IGNORECHECKSERVER(IGNORECHECKSERVER) {}
			void Check();
			inline _Server* getServer() { return srv; }
			inline String getHost() const { return host; }

			void CheckInThread();
			inline bool IsJoinable() { if (thread != nullptr) { return thread->joinable(); } return true; }
			inline void Join() { if (thread != nullptr) { thread->join(); delete thread; thread = nullptr; } }
	};

	class TCPCheckerLoop{
		private:
			List<TCPCheckerWorker * > workers;
			int active_workers = 0;
			bool findet = false;
			std::mutex lopperStatus;
		public:
			TCPCheckerLoop() {};
			~ TCPCheckerLoop() {
				Join();
			}
			inline void Join() {
				for ( auto it = workers.begin(); it != workers.end(); it++) {
					TCPCheckerWorker *  checker = *it;
					checker->Join();
					delete checker;
				}
				workers.clear();
			}
			_Server * Check(List<_Server*> & servers, const String & bootstrapDNS, bool IGNORECHECKSERVER, std::function<String(String)> replaceVariables);

	};

}
