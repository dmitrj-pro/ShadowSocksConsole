#include "Zip.h"
#include <Log/Log.h>
#include "zip_file.hpp"
#include <Types/Exception.h>
#include <_Driver/Path.h>

using __DP_LIB_NAMESPACE__::Path;

minizip_wrapper::minizip_wrapper(const String & filename) {
	zip = new __DP_WWW_GENERATOR_ZIP_FILE_HPP__::miniz_cpp_RANDOM_NAMESPACE_020220221214::zip_file(filename);
}

minizip_wrapper::minizip_wrapper() {
	zip = new __DP_WWW_GENERATOR_ZIP_FILE_HPP__::miniz_cpp_RANDOM_NAMESPACE_020220221214::zip_file();
}

minizip_wrapper::~minizip_wrapper() {
	if (zip != nullptr) {
		zip->reset();
		delete zip;
	}
}

bool minizip_wrapper::has_file(const String & filename) {
	return zip->has_file(filename);
}

List<String> minizip_wrapper::namelist() {
	return zip->namelist();
}

void minizip_wrapper::close() {
	zip->reset();
}

String minizip_wrapper::read(const String & filename) {
	return zip->read(filename);
}

char * minizip_wrapper::read(const String & filename, unsigned long long & size) {
	std::size_t t;
	char * r = zip->read(filename, t);
	size = t;
	return r;
}

void minizip_wrapper::write(const String & filename) {
	zip->write(filename);
}

void minizip_wrapper::write(const String & filename, const String & zip_name) {
	zip->write(filename, zip_name);
}

void minizip_wrapper::save(const String & filename){
	zip->save(filename);
}
Vector<unsigned char> minizip_wrapper::save() {
	Vector<unsigned char> res;
	zip->save(res);
	return res;
}


void ZipReader::open(const String & filename) {
	DP_LOG_DEBUG << "open file " << filename;
	if (opened)
		throw EXCEPTION("Zip file allready opened");
	Path p {filename};
	if (!p.IsFile())
		throw EXCEPTION("File is not exists");
	this->filename = filename;
	zip = SmartPtr<minizip_wrapper>(new minizip_wrapper(this->filename));
	opened = true;
}

List<String> ZipReader::fileList() {
	return zip->namelist();
}

bool ZipReader::hasFile(const String & filename) {
	return zip->has_file(filename);
}

String ZipReader::getAsText(const String & filename) {
	return zip->read(filename);
}

char * ZipReader::getAsBin(const String & filename, unsigned long long & size) {
	return zip->read(filename, size);
}

ZipWriter::ZipWriter() {
	zip = SmartPtr<minizip_wrapper>(new minizip_wrapper());
}

void ZipWriter::addFile(const String & file) {
	zip->write(file);
}

void ZipWriter::addFile(const String & file, const String & asFile) {
	zip->write(file, asFile);
}

bool ZipWriter::hasFile(const String & filename) {
	return zip->has_file(filename);
}

List<String> ZipWriter::fileList() {
	return zip->namelist();
}

void ZipWriter::save(const String & filename) {
	zip->save(filename);
}

Vector<unsigned char> ZipWriter::save() {
	return zip->save();
}
