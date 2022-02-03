#pragma once

#include <DPLib.conf.h>
#include <functional>
#include <_Driver/ThreadWorker.h>
#include <mutex>

using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::List;
using __DP_LIB_NAMESPACE__::UInt;

struct _Server;
struct _Task;

namespace TCPChecker {
	enum class TCPStatus { None, Allow, Disallow};

	class TCPCheckerWorker{
		private:
			std::function<void (TCPStatus, TCPCheckerWorker * )> callback;
			UInt port;
			String host;
			bool IGNORECHECKSERVER;
			String bootstrapDNS;
			_Server* srv = nullptr;
			_Task * task = nullptr;
			__DP_LIB_NAMESPACE__::Thread * thread = nullptr;
		public:
			TCPCheckerWorker(_Task * task,
							 _Server* srv,
							const String & host,
							UInt port,
							 std::function<void (TCPStatus, TCPCheckerWorker * )> callback,
							 const String & bootstrapDNS,
							 bool IGNORECHECKSERVER = false)
				:
					task(task),
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
			inline void Join() { if (thread != nullptr) { thread->join(); delete thread; thread = nullptr; } }
	};

	class TCPCheckerLoop{
		private:
			List<TCPCheckerWorker * > good_workers;
			List<TCPCheckerWorker * > bad_workers;
			int active_workers = 0;
			bool findet = false;
			std::mutex lopperStatus;
		public:
			TCPCheckerLoop() {};
			~ TCPCheckerLoop() {
				Join();
			}
			inline void Join() {
				for ( auto it = good_workers.begin(); it != good_workers.end(); it++) {
					TCPCheckerWorker *  checker = *it;
					checker->Join();
					delete checker;
				}
				good_workers.clear();
				for ( auto it = bad_workers.begin(); it != bad_workers.end(); it++) {
					TCPCheckerWorker *  checker = *it;
					checker->Join();
					delete checker;
				}
				bad_workers.clear();
			}
			// task = null if not need deep check
			_Server * Check(_Task * task, List<_Server*> & servers, const String & bootstrapDNS, bool IGNORECHECKSERVER, std::function<String(String)> replaceVariables);
			List<_Server*> CheckAll(_Task * task, List<_Server*> & servers, const String & bootstrapDNS, bool IGNORECHECKSERVER, std::function<String(String)> replaceVariables);

	};

}
