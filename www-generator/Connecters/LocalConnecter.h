#ifndef __DP_FTP_SYNC_LOCAL_CONNECTER_
#define __DP_FTP_SYNC_LOCAL_CONNECTER_

#include <Types/Exception.h>
#include <DPLib.conf.h>

using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::List;

namespace Connect {
	class LocalConnecter{
		public:
			List<String> GetFiles(const String & remote);
			bool IsDir(const String & file);
			bool Put(const String & local, const String & remote);
			bool Get(const String & remote, const String & local);
			unsigned long FileSize(const String & filepath);
			bool MkDir(const String & filepath);
			bool Connect() { return true; }
			bool IsError() { return false; }
			String GetError() { return ""; }
			bool Close() { return true; }
			bool FileExists(const String & file);
			bool RemoveFile(const String & file);
			bool RemoveDir(const String & dir);
	};
}

#endif
