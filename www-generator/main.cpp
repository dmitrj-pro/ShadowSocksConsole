#include <_Driver/ServiceMain.h>
#include <_Driver/Path.h>
#include <_Driver/Files.h>
#include <unistd.h>
#include <Log/Log.h>
#include <iostream>
#include <limits.h>
#include "Connecters/LocalConnecter.h"
#include "Zip/Zip.h"
#include "Zip/zip-pack.h"
#include <Parser/SmartParser.h>

using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::List;
using __DP_LIB_NAMESPACE__::Path;
using __DP_LIB_NAMESPACE__::Ifstream;
using __DP_LIB_NAMESPACE__::Ofstream;
using __DP_LIB_NAMESPACE__::endWith;
using __DP_LIB_NAMESPACE__::AllRead;
using __DP_LIB_NAMESPACE__::fileSize;

class Generator: public __DP_LIB_NAMESPACE__::ServiceMain {
	private:
		String input = "www";
		String output = "www/www.h";
		List<String> txt_types {".html", ".css", ".js", ".txt", ".map", ".scss"};
		List<String> bin_types {".png", ".jpg", ".ico", ".eot", ".svg", ".ttf", ".woff", ".woff2"};
		bool is_debug = false;
		bool enable_cast = false;
		bool enable_zip = false;

		void runDebug();
		void runRelease();

		void sourceOutput(unsigned int base_path_size, const List<String> & bin_files, const List<String> & txt_files);
		void zipOutput(unsigned int base_path_size, const List<String> & bin_files, const List<String> & txt_files);
	public:
		void MainLoop() {
			if (is_debug)
				runDebug();
			else
				runRelease();
		}
		inline void setDebug() {
			is_debug = true;
		}
		inline void setRelease() {
			is_debug = false;
		}
		inline void setCast() {
			enable_cast = true;
		}
		inline void setZip() {
			enable_zip = true;
		}
		inline void onError() {
			this->SetNeedToExit(true);
			this->SetExitCode(1);
		}
		inline void setWorkFolder(const String & folder) {
			this->input = folder;
		}
		inline void setOutputFile(const String & folder) {
			this->output = folder;
		}
		inline void setTextTypes(const String & types) {
			auto vec = __DP_LIB_NAMESPACE__::split(types, ',');
			this->txt_types.clear();
			for (const String & t: vec)
				this->txt_types.push_back(t);
		}
		inline void setBinTypes(const String & types) {
			auto vec = __DP_LIB_NAMESPACE__::split(types, ',');
			this->bin_types.clear();
			for (const String & t: vec)
				this->bin_types.push_back(t);
		}

		void MainExit() { }
		void PreStart(){
			DP_SM_addArgumentHelp0(&Generator::setDebug, "Write debug version", "-d", "--d", "-debug", "--debug", "debug");
			DP_SM_addArgumentHelp0(&Generator::setRelease, "Write release version", "-r", "--r", "-release", "--release", "release");
			DP_SM_addArgumentHelp0(&Generator::setCast, "Write bin data with cast (Release only)", "-c", "--c", "-cast", "--cast", "cast");
			DP_SM_addArgumentHelp0(&Generator::setZip, "Zip compress file", "-z", "--z", "-zip", "--zip", "zip");
			DP_SM_addArgumentHelp1(String, &Generator::setWorkFolder, &Generator::onError, "Set work directory default www", "-i", "--i", "-input", "--input", "input");
			DP_SM_addArgumentHelp1(String, &Generator::setOutputFile, &Generator::onError, "Set output file default www/www.h", "-o", "--o", "-out", "--out", "out");
			DP_SM_addArgumentHelp1(String, &Generator::setTextTypes, &Generator::onError, "Set text types for search (Ex. .html,.css,.js,.txt,.map,.scss", "-t", "--t", "-text-types", "--text-types", "text-types");
			DP_SM_addArgumentHelp1(String, &Generator::setBinTypes, &Generator::onError, "Set bin types for search (Ex. .png,.jpg,.ico,.eot,.svg,.ttf,.woff,.woff2", "-b", "--b", "-bin-types", "--bin-types", "bin-types");

			AddHelp(std::cout, [this]() { this->SetNeedToExit(true); }, "help", "-help", "--help", "-h", "--h", "?");
		}
};


String ReplaceAllSubstringOccurrences(String & sAll, const String & sStringToRemove, const String & sStringToInsert) {
   int iLength = sStringToRemove.length();
   size_t index = 0;
   while (true) {
	  /* Locate the substring to replace. */
	  index = sAll.find(sStringToRemove, index);
	  if (index == std::string::npos)
		 break;

	  /* Make the replacement. */
	  sAll.replace(index, iLength, sStringToInsert);

	  /* Advance index forward so the next iteration doesn't pick it up as well. */
	  index += iLength;
   }
   return sAll;
}

void Generator::runDebug() {
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == nullptr)
		return;
	Path _cur (cwd);
	{

		Path p {input};
		if (p.IsAbsolute())
			_cur = p;
		else
			_cur.Append(input);
	}

	String l_path = _cur.Get();
	#ifdef DP_WIN
		ReplaceAllSubstringOccurrences(l_path, "\\", "/");
	#endif
	DP_LOG_INFO << "Worker dir " << _cur.Get();
	Ofstream out;
	out.open(output);
	out << R"""(#pragma once
#include <DPLib.conf.h>
#include <_Driver/Files.h>
#include <_Driver/Path.h>

using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::FileExists;
using __DP_LIB_NAMESPACE__::Path;
using __DP_LIB_NAMESPACE__::AllRead;
using __DP_LIB_NAMESPACE__::Ifstream;

inline std::string findCode(const std::string & file) {
	Path this_file = Path(")""" << l_path << R"""(");
	this_file.Append(file);
	if (this_file.IsFile()) {
		return AllRead(this_file.Get());
	}
	return "";
}

inline char * findResource(const std::string & file, unsigned long long & size) {
	Path this_file = Path(")""" << l_path << R"""(");
	this_file.Append(file);
	if (!this_file.IsFile())
		return nullptr;
	size = __DP_LIB_NAMESPACE__::fileSize(this_file.Get());
	char * res = new char[size + 1];

	Ifstream in;
	in.open(this_file.Get(), std::ios_base::binary);

	in.read(res, size);
	in.close();
	return res;
})""";
	out.close();
}


inline char ByteToHex(unsigned short b){
	if ( (b < 10 ) )
		return (char) ('0' + b);
	switch (b) {
		case 10:
			return 'a';
			break;
		case 11:
			return 'b';
			break;
		case 12:
			return 'c';
			break;
		case 13:
			return 'd';
			break;
		case 14:
			return 'e';
			break;
		case 15:
			return 'f';
			break;
	}
	throw EXCEPTION("Try convert integer > 15");
}


void Generator::runRelease() {
	List<String> folders;
	List<String> bin_files;
	List<String> txt_files;

	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == nullptr)
		return;
	Path cur (cwd);
	{

		Path p {input};
		if (p.IsAbsolute())
			cur = p;
		else
			cur.Append(input);
		Path p2 {output};
		if (p2.IsFile())
			__DP_LIB_NAMESPACE__::RemoveFile(p2.Get());
	}
	folders.push_back(cur.Get());

	Connect::LocalConnecter conn;

	while(folders.size() > 0) {
		String folder = *folders.begin();
		folders.pop_front();
		DP_LOG_INFO << "Current dir " << folder;

		auto list = conn.GetFiles(folder);
		for (String file : list) {
			Path p = Path(folder);
			p.Append(file);
			if (conn.IsDir(p.Get())) {
				folders.push_back(p.Get());
			} else {
				for (String t : this->txt_types)
					if (endWith(p.Get(), t)) {
						txt_files.push_back(p.Get());
						break;
					}
				for (String t : this->bin_types)
					if (endWith(p.Get(), t)) {
						bin_files.push_back(p.Get());
						break;
					}
			}
		}
	}

	if (!enable_zip)
		sourceOutput(cur.Get().size(), bin_files, txt_files);
	else
		zipOutput(cur.Get().size(), bin_files, txt_files);
}

void Generator::sourceOutput(unsigned int base_path_size, const List<String> & bin_files, const List<String> & txt_files) {
	Ofstream out;
	out.open(output);
	out << R"""(#pragma once
#define INIT_ZIP_HEADER
#include <string>
#include <cstring>

inline std::string findCode(const std::string & file) {
)""";

	for (String file : txt_files) {
		String l_path = file;
		if (file.size() > base_path_size)
			l_path = file.substr(base_path_size + 1);
		#ifdef DP_WIN
			ReplaceAllSubstringOccurrences(l_path, "\\", "/");
		#endif
		DP_LOG_INFO << "Current text file " << file << " (" << l_path << ")";
		out << "    if (file == \"" << l_path << "\") {\n" << "        return R\"\"\"(" << AllRead(file) << ")\"\"\";\n" << "    }\n";
	}
	out << "    return \"\";\n}\ninline char * findResource(const std::string & file, unsigned long long & size) {\n";

	for (String file : bin_files) {
		String l_path = file;
		if (file.size() > base_path_size)
			l_path = file.substr(base_path_size + 1);

		#ifdef DP_WIN
			ReplaceAllSubstringOccurrences(l_path, "\\", "/");
		#endif
		DP_LOG_INFO << "Current bin file " << file << " (" << l_path << ")";
		Ifstream in;
		in.open(file, std::ios_base::binary);
		const unsigned int buffer_size = 1;
		char buff[buffer_size];

		out << "    if (file == \"" << l_path << "\") {\n" << "        size = " << fileSize(file) << ";\n        char * r = new char [size + 1];\n        static char res[] {";
		while (in.read(buff, buffer_size)) {
			unsigned short v = (unsigned char) buff[0];
			out << (enable_cast ? "(char)" : "") <<  "0x" << ByteToHex(v/16) << ByteToHex(v%16) << ", ";
		}
		out << "0x0 };\n";
		out << "        memcpy(r, res, size);\n        return r;\n    }\n";
	}
	out << "    return nullptr; \n}";
	out.close();
}

void Generator::zipOutput(unsigned int base_path_size, const List<String> & bin_files, const List<String> & txt_files) {
	ZipWriter zip;

	for (String file : txt_files) {
		String l_path = file;
		if (file.size() > base_path_size)
			l_path = file.substr(base_path_size + 1);
		#ifdef DP_WIN
			ReplaceAllSubstringOccurrences(l_path, "\\", "/");
		#endif
		DP_LOG_INFO << "Current text file " << file << " (" << l_path << ")";
		zip.addFile(file, l_path);
	}

	for (String file : bin_files) {
		String l_path = file;
		if (file.size() > base_path_size)
			l_path = file.substr(base_path_size + 1);

		#ifdef DP_WIN
			ReplaceAllSubstringOccurrences(l_path, "\\", "/");
		#endif
		DP_LOG_INFO << "Current bin file " << file << " (" << l_path << ")";
		zip.addFile(file, l_path);
	}
	auto zipped = zip.save();


	Ofstream out;
	out.open(output);
	out << "#define INIT_ZIP_HEADER ZipReader ZipReader::reader{};\n" << findCode("zip_file.hpp");
	out << R"""(
		   class ZipReader{
			   private:
				   __DP_WWW_GENERATOR_ZIP_FILE_HPP__::miniz_cpp_RANDOM_NAMESPACE_020220221214::zip_file * zip;
				   static ZipReader reader;

			   public:
				   static inline ZipReader & Get() { return reader; }
				   ZipReader(){
						zip = new __DP_WWW_GENERATOR_ZIP_FILE_HPP__::miniz_cpp_RANDOM_NAMESPACE_020220221214::zip_file(
								std::vector<unsigned char>{
									)""";
	unsigned long long i = 0;
	for (unsigned char c : zipped) {
		out << (i == 0 ? "" : ", ") << (enable_cast ? "(unsigned char)" : "") <<  "0x" << ByteToHex(c/16) << ByteToHex(c%16);
		i++;
	}

out << R"""(								}
							);

				   }

				   inline std::list<std::string> fileList() { return zip->namelist(); }
				   inline bool hasFile(const std::string & filename) { return zip->has_file(filename); }
				   inline std::string getAsText(const std::string & filename) { return zip->read(filename); }
				   inline char * getAsBin(const std::string & filename, unsigned long long & size) { std::size_t t; char * r = zip->read(filename, t); size = t; return r; }
		   };
		   inline std::string findCode(const std::string & file) {
				if (ZipReader::Get().hasFile(file))
					return ZipReader::Get().getAsText(file);
				return "";
		   }
		   inline char * findResource(const std::string & file, unsigned long long & size) {
			   if (ZipReader::Get().hasFile(file)) {
					return  ZipReader::Get().getAsBin(file, size);
			   }
			   return nullptr;
		   }
		   )""";
	out.close();
}

DP_ADD_MAIN_FUNCTION(new Generator());
