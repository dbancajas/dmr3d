#!/bin/sh


files=`find . -name "*.cc"`

for f in $files
do
	~/bin/remove_RCS.pl $f
done
