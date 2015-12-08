#!/bin/bash
# Driver for jsi test programs.
# Runs *.js files in tests/ and compares output to EXPECTSTART/END in file comment.
RUNVALG=0
QUIET=0
LOUD=0
JSISH=./jsish
INPSTR="input\ntest\nabc@gmail.com\n" 
USESTR="Usage: testjs.sh [-loud|-quiet|-memcheck] <dir|file>"

while [ $# -gt 1 ]; do
    case $1 in
        "-memcheck") RUNVALG=1; shift;;
        "-loud") LOUD=1; shift;;
        "-quiet") QUIET=1; shift;;
        "-input") INPSTR=$2; shift; shift;;
        -*)
            if [ ! -e "$1" ]; then
                echo "Unknown option: $1."
                echo $USESTR
                exit 1
            fi
            break;;
        *)
            break;;
    esac
done

if [ $# != 1 ]; then
    echo "Too many args at: $*."
    echo $USESTR
    exit 1
fi
if [ ! -e "$1" ]; then
    echo "File or directory expected: $1."
    echo $USESTR
    exit 1
fi

function test1file()
{
    file=$1
        awk '{ 
            if ($0 ~ /^=\!EXPECTSTART\!=$/) start = 1;
            else if ($0 ~ /=\!EXPECTEND\!=/) start = 0;
            else if (start) print 
        }' $file >/tmp/ss_expected
        
        echo -e $INPSTR | "$JSISH" $file g 2 3 2>&1 >/tmp/ss_result
        ORC=$?
        if [ $ORC != 0 ]; then
             echo "[ BADRC  ]     $file: RC=$ORC"
             return;
        fi
        
        if ! diff /tmp/ss_expected /tmp/ss_result >/dev/null; then
            DATA=`cat /tmp/ss_expected`
            if [ "$DATA" == "" ]; then CM='*'; else CM=' '; fi
            echo -e "[ FAILED ]  $CM  $file"
            if [ $LOUD != 0 ]; then
                echo "============expected============="
                cat /tmp/ss_expected
                echo "------------result---------------"
                cat /tmp/ss_result
                echo "-------------diff----------------"
                diff -u /tmp/ss_expected /tmp/ss_result | tail -n +3
                echo "==============end==============="
                return
            fi
            if [ $QUIET != 1 ]; then
                echo "-------------diff----------------"
                diff -u /tmp/ss_expected /tmp/ss_result | tail -n +3
                return
            fi
        else
            echo -e "[   OK   ]     $file"
        fi
}

function testdir()
{
    allfiles=`find "$1" -maxdepth 1 -name "*.js" | sort -r`
    for file in $allfiles; do
        echo "Executing $file"
        test1file $file $2
    done
}

function runvalgind()
{
    allfiles=`find "$1" -name "*.js"`
    rm -f valgrind.report.txt
    
    for file in $allfiles; do
        echo -n "Executing $file"
        echo "**************** $file ******************" >>valgrind.report.txt
        f=$(basename $file)
        if [ "$f" != "block.js" -a "$f" != "block2.js" ]; then
            echo -e "input\ntest\npmacdona@gmail.com\n" | valgrind "$JSISH" $file 1 2 3 >>valgrind.report.txt 2>&1
        fi
        echo -e "[  DONE  ]     $file"
    done
    echo "All done, report: valgrind.report.txt"
}

if [ "$2" == "-m" ]; then
    runvalgind "$1"
    return
fi
if [ -d "$1" ]; then
    testdir "$1" "$2"
else
    test1file "$1" "$2"
fi
