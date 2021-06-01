#!/bin/bash

dir=$(echo $0 | xargs dirname)

maxDate=$(date +"%s" -d 2000-01-01)
maxValue="0.0.1"


ls $dir | while read -r line; do
	if [[ ! $line == "version"* ]]; then
		file="$dir/$line"
		date_mod=$(stat "$file" | grep Modi)
		date_mod=${date_mod:8:10}
		date_mod=$(date +"%s" -d $date_mod)
		if (( "$date_mod" >= "$maxDate" )); then
			echo $line
		fi
	fi
done | tail -f -n1