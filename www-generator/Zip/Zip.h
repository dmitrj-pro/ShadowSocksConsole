#pragma once
#include <DPLib.conf.h>
#include <Types/SmartPtr.h>

using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::List;
using __DP_LIB_NAMESPACE__::Vector;
using __DP_LIB_NAMESPACE__::Char;
using __DP_LIB_NAMESPACE__::SmartPtr;

namespace __DP_WWW_GENERATOR_ZIP_FILE_HPP__{
	namespace miniz_cpp_RANDOM_NAMESPACE_020220221214{
		class zip_file;
	}
}

class minizip_wrapper{
	private:
		__DP_WWW_GENERATOR_ZIP_FILE_HPP__::miniz_cpp_RANDOM_NAMESPACE_020220221214::zip_file * zip = nullptr;
	public:
		minizip_wrapper();
		minizip_wrapper(const String & filename);
		~minizip_wrapper();
		List<String> namelist();
		bool has_file(const String & filename);
		String read(const String & filename);
		void close();
		char * read(const String & filename, unsigned long long & size);
		void write(const String & filename);
		void write(const String & filename, const String & zip_name);
		void save(const String & filename);
		Vector<unsigned char> save();
};

class ZipReader{
	private:
		String filename = "";
		bool opened = false;
		SmartPtr<minizip_wrapper> zip;

	public:
		ZipReader(){}
		ZipReader(const String &filename) { this->open(filename); }

		void open(const String & filename);
		inline void close() { zip->close(); opened = false; zip = SmartPtr<minizip_wrapper>(nullptr); }
		void reopen() {
			if (opened) return;
			open(filename);
		}
		inline bool is_open() const { return opened; }
		inline String Filename() const { return filename; }

		List<String> fileList();
		bool hasFile(const String & filename);
		String getAsText(const String & filename);
		char * getAsBin(const String & filename, unsigned long long & size);
};

class ZipWriter{
	private:
		SmartPtr<minizip_wrapper> zip;
	public:
		ZipWriter();
		~ZipWriter() {}

		void addFile(const String & file);
		void addFile(const String & file, const String & asFile);
		bool hasFile(const String & filename);
		List<String> fileList();

		void save(const String & filename);
		Vector<unsigned char> save();
};
