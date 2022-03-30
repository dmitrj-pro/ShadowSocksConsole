#!/bin/bash

# MIME:
# cat mime.types | awk '{ printf "{\""; printf $2 ; printf "\", \"" ; printf $1; print "\"}," }'


if [ "$WINDIR" ]; then
	path_to_file=$(realpath www/textfill.h | sed 's/\///p' | sed 's/\//:\\\\/' | sed 's/\//\\\\/g' | tail -n1)
else
	path_to_file=$(realpath "www/textfill.h")
fi

echo "Base path is "$path_to_file

cpp=$(cat <<EOF
#pragma once
#include <DPLib.conf.h>
#include <_Driver/Files.h>
#include <_Driver/Path.h>

using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::FileExists;
using __DP_LIB_NAMESPACE__::Path;
using __DP_LIB_NAMESPACE__::AllRead;
using __DP_LIB_NAMESPACE__::Ifstream;

inline std::string findCode(const std::string & file) {
	Path this_file = Path("${path_to_file}");
	this_file = Path(this_file.GetFolder());
	this_file.Append(file);
	if (this_file.IsFile()) {
		return AllRead(this_file.Get());
	}
	return "";
}


inline char * findResource(const std::string & file, unsigned long long & size) {
	Path this_file = Path("${path_to_file}");
	this_file = Path(this_file.GetFolder());
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
}
EOF
)

echo "$cpp" > www/www.h