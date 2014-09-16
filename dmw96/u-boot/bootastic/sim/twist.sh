#!/bin/bash

while read line; do
	size=${#line}

	let size=size-2

	while [ $size -ge 0 ]; do
		printf ${line:size:2}
		let size=size-2
	done

	printf "\n"
done < $1

