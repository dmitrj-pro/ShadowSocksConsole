#include "Finder.h"
#include <dirent.h>
#include <string.h>
#include <iostream>
#include <Parser/SmartParser.h>
#include <sys/stat.h>
#include <Types/Exception.h>
#include <cstdio>
#include <unistd.h>

using std::cout;
using __DP_LIB_NAMESPACE__::Ifstream;
using __DP_LIB_NAMESPACE__::Ofstream;
using __DP_LIB_NAMESPACE__::UInt;


namespace Files {
	unsigned long fileSize(const String & file) {
		Ifstream in;
		in.open(file, std::ios::ate | std::ios::binary);
		return in.tellg();
	}

	bool isDir(const String & folder) {
		DIR *dp;
		struct dirent *dent;

		if( (dp = opendir(folder.c_str())) == NULL) {
			return false;
		}
		closedir(dp);
		return true;
	}
	bool RemoveFile(const String & path) {
		if (remove(path.c_str()) != 0)
			return false;
		return true;
	}

	bool RemoveDir(const String & path) {
		if (rmdir(path.c_str()) != 0)
			return false;
		return true;
	}

	bool MkDir(const String & folder) {
		#ifdef _WIN32
			if (mkdir(folder.c_str()) != 0) {
		#endif
		#ifndef _WIN32
			if (-1 == mkdir(folder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
		#endif
			return false;
		}
		return true;
	}

	list <String> GetFiles(const String & folder){
		DIR *dp;
		struct dirent *dent;

		if( (dp = opendir(folder.c_str())) == NULL) {
			cout << "opendir: " << folder << "\n";
			return list<String>();
		}
		list<String> all;
		while(dent = readdir(dp))
			if(strcmp(".", dent->d_name) && strcmp("..", dent->d_name))
				all.push_back(dent->d_name);
		closedir(dp);
		return all;
	}

	list<String> SetFilter(const list<String> & data, const String& filter){
		static __DP_LIB_NAMESPACE__::SmartParser par(filter);

		list<String> res;
		for (auto x = data.cbegin(); x != data.cend(); x++){
			if (par.Check(*x)){
				res.push_back(*x);
			}
		}
		return res;
	}
	bool CopyFile(const String & from, const String & to) {
		Ifstream in;
		in.open(from, std::ios::binary);
		if (in.fail())
			throw EXCEPTION("Files " + from + " is not found");

		Ofstream out;
		out.open(to, std::ios::binary);
		if (out.fail())
			throw EXCEPTION("Files " + from + " is not found");

		__DP_LIB_NAMESPACE__::Char c;
		while (in.get(c)) {
			out.put(c);
		}
		out.close();
		in.close();
		return true;
	}

	String GetPath(const String & file) {
		UInt pos = 0;
		for (Int i = file.size() -1; i>=0; i--)
			if (file[i] == '\\' || file[i] =='/') {
				pos = i;
				break;
			}

		if (pos == 0)
			return "";
		String res = file.substr(0, pos);
		return res;
	}
	String GetName(const String & file) {
		UInt pos = 0;
		for (Int i = file.size() -1; i>=0; i--)
			if (file[i] == '\\' || file[i] =='/') {
				pos = i;
				break;
			}

		if (pos == 0)
			return file;
		String res = file.substr(pos+1);
		return res;
	}
}
