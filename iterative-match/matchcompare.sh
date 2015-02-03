#!/bin/bash

set -e

mypath=`dirname $0`

sonic-annotator --minversion 1.1 || exit 1

reference="$1"
other="$2"

usage() {
    echo "Usage: $0 reference.wav other.wav" 1>&2
    exit 2
}

test -n "$reference" || usage
test -n "$other" || usage

set -u

transform="/tmp/$$.ttl"
scores="/tmp/$$.scores.txt"
trap "rm $transform $scores" 0

#export VAMP_PATH="$mypath"/../match-vamp

sox "$reference" -r 44100 -c 1 a.wav
sox "$other" -r 44100 -c 1 b.wav

for cents in $(seq -400 10 400) ; do

    freq=$(perl -e "print 440.0 * (2.0 ** ($cents / 1200.0))")

    cat match-cost.ttl | sed "s,440,$freq," > "$transform"

    score=$(sonic-annotator -t "$transform" --multiplex -w csv --csv-stdout a.wav b.wav | awk -F, '{ print $3 }')

    echo "$score $freq"
    
done | tee "$scores"

echo
echo Scores:
sort -n "$scores"

echo
bestfreq=$(sort -n "$scores" | head -1 | awk '{ print $2; }')
echo "I reckon the tuning frequency is $bestfreq"

