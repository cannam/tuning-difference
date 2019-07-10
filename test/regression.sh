#!/bin/bash

set -eu

mydir=$(dirname "$0")

source_url=https://code.soundsoftware.ac.uk/attachments/download/1698/Zweieck-Duell.ogg

testfile="$mydir/input.ogg"

if sonic-annotator -v >/dev/null ; then
    :
else
    echo "Failed to find required binary sonic-annotator"
    exit 1
fi

if oggdec -h >/dev/null ; then
    :
else
    echo "Failed to find required binary oggdec"
    exit 1
fi

if rubberband -h 2>&1 | grep Particular >/dev/null ; then
    :
else
    echo "Failed to find required binary rubberband"
    exit 1
fi

if wget --version >/dev/null ; then
    wget -O "$testfile" "$source_url"
else
    curl -o "$testfile" "$source_url"
fi

wavfile=${testfile%%.ogg}.wav
lowfile=${testfile%%.ogg}-low.wav

oggdec -o "$wavfile" "$testfile"

rubberband -p -2.34 "$wavfile" "$lowfile"

VAMP_PATH="$mydir/.." \
         time \
	 sonic-annotator \
         -t "$mydir/tests.ttl" \
	 -w csv \
         --csv-basedir "$mydir/output" \
	 --csv-force \
	 --csv-omit-filename \
         --multiplex \
	 "$testfile" \
	 "$lowfile"

failed=""

for expected in "$mydir"/expected/*.csv ; do
    outfile="$mydir"/output/$(basename $expected)
    if cmp "$outfile" "$expected" ; then
        echo "PASS: $outfile"
    else
        echo
        echo "*** FAIL: Result does not match expected output. Diff follows:"
        echo
        sdiff -w 60 "$outfile" "$expected"
        echo
        failed="$failed $outfile"
    fi
done

if [ -n "$failed" ]; then
    echo "Some tests failed: $failed"
    exit 1
else
    echo "All tests passed"
    exit 0
fi


