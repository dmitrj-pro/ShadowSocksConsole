#pragma once

#include <DPLib.conf.h>
#include "ShadowSocksMain.h"
#include <Generator/CodeGenerator.h>
#include <functional>
#include "ShadowSocks.h"

using __DP_LIB_NAMESPACE__::List;

class _ShadowSocksController {
	private:
		ShadowSocksSettings settings;
		String password = "";
		List<__DP_LIB_NAMESPACE__::Pair<int, ShadowSocksClient *> > clients;
		std::function<void()> _make_exit;
		// 0 - Unknow
		// 1 - AutoStart
		// 2 - Disable AutoStart
		int mode = 0;
	public:
		inline void EnableAutoStart() { mode = 1; }
		inline void DisableAutoStart() { mode = 2; }
		inline const List<__DP_LIB_NAMESPACE__::Pair<int, ShadowSocksClient *> > & getRunning() const { return clients; }
		void Stop();
		String GetConfigPath();
		inline ShadowSocksSettings & getConfig() { return settings; }
		String GetSourceConfig(const String & filename, const String & password);
		String GetSourceConfig(const String & password);
		String GetSourceConfig();
		void OpenConfig(const String & password);
		void OpenConfig();
		void SaveConfig();
		void SaveConfig(const String & config);
		bool isEncrypted();
		bool isCreated();
		inline bool isOpened() const { return password.size() > 1; }
		void SetPassword(const String & password);
		inline void MakeExit() { Stop(); _make_exit(); }
		inline void SetExitFinc(std::function<void()> func) { _make_exit = func; }
		void AutoStart(std::function<void()> onCrash);
		void StartById(int id, std::function<void()> onSuccess, std::function<void()> onCrash, SSClientFlags flags);
		void StartByName(const String & name, std::function<void()> onSuccess, std::function<void()> onCrash, SSClientFlags flags);
		void StopByName(const String & name);
		bool CheckInstall();
};

String ReadAllFile(const String & file);

DP_SINGLTONE_CLASS(_ShadowSocksController, ShadowSocksController, Get, GetRef, Create)
