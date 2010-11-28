#!/bin/bash

RUNVALG=0

if [ $# -lt 2 ]; then
	echo "Usage: runtest.sh <execute> <testsuit dir> [vg|-v]"
	exit 1
fi

if [ ! -x "$1" ]; then
	echo "$1 is not executable"
	exit 1
fi

if [ ! -d "$2" ]; then
	echo "$2 is not a directory"
	exit 1
fi

function functionaltest()
{
	allfiles=`find "$2" -name "*.ss"`
	for file in $allfiles; do
		echo -n "Executing $file"
		awk '{ 
			if ($0 ~ /^=\!EXPECTSTART\!=$/) start = 1;
			else if ($0 ~ /=\!EXPECTEND\!=/) start = 0;
			else if (start) print 
		}' $file >/tmp/ss_expected
		
		echo -e "input\ntest\nwenxichang@163.com\n" | "$1" $file 1 2 3 >/tmp/ss_result 2>/dev/null
		
		if ! diff /tmp/ss_expected /tmp/ss_result >/dev/null; then
			DATA=`cat /tmp/ss_expected`
			if [ "$DATA" == "" ]; then CM='*'; else CM=' '; fi
			echo -e "\r[ FAILED ]  $CM  $file"
			if [ "$3" == "-v" ]; then
				echo "============compare============="
				cat /tmp/ss_expected
				echo "--------------------------------"
				cat /tmp/ss_result
				echo "==============end==============="
			fi
		else
			echo -e "\r[   OK   ]     $file"
		fi
	done
}

function runvalgind()
{
	allfiles=`find "$2" -name "*.ss"`
	rm -f valgrind.report.txt
	
	for file in $allfiles; do
		echo -n "Executing $file"
		echo "**************** $file ******************" >>valgrind.report.txt
		f=$(basename $file)
		if [ "$f" != "block.ss" -a "$f" != "block2.ss" ]; then
			echo -e "input\ntest\nwenxichang@163.com\n" | valgrind "$1" $file 1 2 3 >>valgrind.report.txt 2>&1
		fi
		echo -e "\r[  DONE  ]     $file"
	done
	echo "All done, report: valgrind.report.txt"
}

if [ "$3" == "vg" ]; then
	runvalgind "$1" "$2"
else
	functionaltest "$1" "$2" "$3"
fi
