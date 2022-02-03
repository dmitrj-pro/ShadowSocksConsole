#!/bin/bash

# MIME:
# cat mime.types | awk '{ printf "{\""; printf $2 ; printf "\", \"" ; printf $1; print "\"}," }'


cpp=$(cat <<EOF
#pragma once
#include <string>
#include <cstring>

inline std::string findCode(const std::string & file) {
EOF
)

echo "$cpp" > Zip/zip-pack.h

filelist=$(find ./Zip/ -name "*.hpp" | sed 's/.\/Zip\///')

while read -r line; do
	echo "Generate Zip/${line}"
	data=$(cat "Zip/${line}")
	cpp=$(cat <<EOF
		if (file == "${line}") {
			return R"""(${data})""";
		}
EOF
)
	echo "$cpp" >> Zip/zip-pack.h
done <<< "$filelist"

echo "    return \"\";" >> Zip/zip-pack.h
echo "}" >> Zip/zip-pack.h
