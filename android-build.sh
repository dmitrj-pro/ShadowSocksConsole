#!/bin/bash

export NDK_PROJECT_PATH=.
rm -rf ./libs/
~/tmp/android-ndk-r23b/ndk-build NDK_APPLICATION_MK=./Application.mk

files=$(find ./libs/ -name "*le")

while read -r line; do
	bname=$(basename $line)
	dname=$(dirname $line)
	mv $line "${dname}/lib${bname}.so"
	printf "$line => ${dname}/lib${bname}.so\n"
done <<< "$files"
