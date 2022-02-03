#include "LocalConnecter.h"
#include "Finder.h"
#include <_Driver/Files.h>

namespace Connect {
	List<String> LocalConnecter::GetFiles(const String & remote) {
		return Files::GetFiles(remote);
	}

	bool LocalConnecter::IsDir(const String & file) {
		return Files::isDir(file);
	}

	unsigned long LocalConnecter::FileSize(const String & filepath) {
		return Files::fileSize(filepath);
	}

	bool LocalConnecter::MkDir(const String & filepath) {
		return Files::MkDir(filepath);
	}

	bool LocalConnecter::Put(const String & local, const String & remote) {
		return Files::CopyFile(local, remote);
	}
	bool LocalConnecter::Get(const String & remote, const String & local) {
		return Files::CopyFile(remote, local);
	}
	bool LocalConnecter::FileExists(const String & file) {
		return __DP_LIB_NAMESPACE__::FileExists(file);
	}
	bool LocalConnecter::RemoveFile(const String & file) {
		return Files::RemoveFile(file);
	}
	bool LocalConnecter::RemoveDir(const String & dir) {
		List<String> elements = GetFiles(dir);
		for (auto it = elements.cbegin(); it != elements.cend(); it ++) {
			String path = dir + "/" + *it;
			if (IsDir(path))
				RemoveDir(path);
			else
				RemoveFile(path);
		}

		return Files::RemoveDir(dir);
	}
}
