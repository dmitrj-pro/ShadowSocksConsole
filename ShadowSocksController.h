#pragma once

#include <DPLib.conf.h>
#include "ShadowSocksMain.h"
#include <Generator/CodeGenerator.h>
#include <functional>
#include "ShadowSocks.h"
#include <_Driver/ThreadWorker.h>
#include <_Driver/Path.h>

using __DP_LIB_NAMESPACE__::List;

// На интерфейс кидаем информацию о статусе заданий
class ShadowSocksControllerUpdateStatus{
	public:
		virtual void UpdateServerStatus(const String & server, const String & msg) = 0;
		virtual void UpdateTaskStatus(const String & server, const String & msg) = 0;
		inline virtual ~ShadowSocksControllerUpdateStatus() {}
};

class _ShadowSocksController {
	private:
		ShadowSocksSettings settings;
		String password = "";
		String __sha1_config_file = "";
		List<__DP_LIB_NAMESPACE__::Pair<int, ShadowSocksClient *> > clients;
		std::mutex clients_lock;
		std::function<void()> _make_exit;
		// 0 - Unknow
		// 1 - AutoStart
		// 2 - Disable AutoStart
		int mode = 0;

		bool need_to_exit = false;
		__DP_LIB_NAMESPACE__::Thread * checkerThread = nullptr;
		List<ShadowSocksControllerUpdateStatus *> notify_update;
		std::mutex notify_update_lock;

		inline void makeNotifyServer(const String & server, const String & msg) {
			notify_update_lock.lock();
			for (ShadowSocksControllerUpdateStatus * n : notify_update)
				n->UpdateServerStatus(server, msg);
			notify_update_lock.unlock();
		}
		inline void makeNotifyTask(const String & task, const String & msg) {
			notify_update_lock.lock();
			for (ShadowSocksControllerUpdateStatus * n : notify_update)
				n->UpdateTaskStatus(task, msg);
			notify_update_lock.unlock();
		}
	public:
		inline void EnableAutoStart() { mode = 1; }
		inline void DisableAutoStart() { mode = 2; }
		inline const List<__DP_LIB_NAMESPACE__::Pair<int, ShadowSocksClient *> > & getRunning() const { return clients; }
		void Stop();
		String GetConfigPath();
		String GetCashePath();
		inline ShadowSocksSettings & getConfig() { return settings; }
		String GetSourceConfig(const String & filename, const String & password);
		String GetSourceConfig(const String & password);
		String GetSourceConfig();
		void OpenConfig(const String & password);
		void OpenConfig();
		void SaveConfig();
		void SaveCashe();
		void ExportConfig(__DP_LIB_NAMESPACE__::Ostream & out, bool is_mobile, bool resolve_dns, const String & default_dns = "94.140.14.14", const String & v2ray_path = "v2ray-plugin");
		void OpenCashe();
		void OpenLogFile();
		void CloseLogFile();
		void SaveConfig(const String & config);
		bool isEncrypted();
		bool isCreated();
		inline bool isOpened() const { return password.size() > 1; }
		void SetPassword(const String & password);
		inline void MakeExit() { Stop(); _make_exit(); }
		inline void SetExitFinc(std::function<void()> func) { _make_exit = func; }
		void AutoStart(OnShadowSocksError onCrash);
		void StartOnBoot();
		void SaveBootConfig();
		ShadowSocksClient * StartById(int id, OnShadowSocksRunned onSuccess, OnShadowSocksError onCrash, SSClientFlags flags);
		ShadowSocksClient * StartByName(const String & name, OnShadowSocksRunned onSuccess, OnShadowSocksError onCrash, SSClientFlags flags);
		void StopByName(const String & name);
		void StopByClient(ShadowSocksClient * client);
		bool isRunning(int id);
		bool CheckInstall();

		struct CheckLoopStruct{
			unsigned int auto_check_interval_s = 60;
			String _server_name = "";
			String _task_name = "";
			bool save_last_check = true;
			AutoCheckingMode auto_check_mode = AutoCheckingMode::Ip;
			String defaultHost = "127.0.0.1";
			String tempPath = "./";
			String wgetPath = "wget";
			String downloadUrl;
			String checkIpUrl;
			bool one_loop = true;
		};
		CheckLoopStruct makeCheckStruct() const;

		void check_loop(const CheckLoopStruct & args);

		void check_server(_Server * srv, const _Task * task,
						  const CheckLoopStruct & args);

		inline void startCheckerThread() {
			if (checkerThread != nullptr) return;
			__DP_LIB_NAMESPACE__::Path cpath = __DP_LIB_NAMESPACE__::Path(GetCashePath());
			if (cpath.IsFile())
				OpenCashe();
			#ifdef DP_ANDROID
				return;
			#endif
			if (this->settings.auto_check_mode != AutoCheckingMode::Off && this->settings.auto_check_mode != AutoCheckingMode::Passiv) {
				checkerThread = new __DP_LIB_NAMESPACE__::Thread([this] () {
					CheckLoopStruct args = makeCheckStruct();
					args.one_loop = false;
					this->check_loop(args);
				});
				checkerThread->SetName("_ShadowSocksController::check_loop");
				checkerThread->start();
			}
		}
		inline void connectNotify(ShadowSocksControllerUpdateStatus * cl) { notify_update_lock.lock();
																			notify_update.push_back(cl);
																			notify_update_lock.unlock(); }
		inline void disconnectNotify(ShadowSocksControllerUpdateStatus * cl) {
			notify_update_lock.lock();
			for (auto it = notify_update.begin(); it != notify_update.end(); it++)
				if ((*it) == cl) {
					notify_update.erase(it);
					break;
				}
			notify_update_lock.unlock();
		}

};

String ReadAllFile(const String & file);

DP_SINGLTONE_CLASS(_ShadowSocksController, ShadowSocksController, Get, GetRef, Create)
