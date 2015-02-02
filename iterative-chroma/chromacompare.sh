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

if ! test -f "$reference" ; then
    echo "Reference file $reference not found" 1>&2
    exit 1
fi

if ! test -f "$other" ; then
    echo "Other file $other not found" 1>&2
    exit 1
fi

refchroma="/tmp/$$.ref"
otherchroma="/tmp/$$.other"
tmpscript="/tmp/$$.dc"
transform="/tmp/$$.ttl"
scores="/tmp/$$.scores.txt"
trap "rm $refchroma $otherchroma $tmpscript $transform $scores" 0

extract() {
    hz="$1"
    file="$2"
    cat chroma-excerpt.ttl | sed 's,"440","'"$hz"'",' > "$transform"
    sonic-annotator -t "$transform" --summary mean --summary-only -w csv --csv-stdout --csv-omit-filename "$file" 2>/dev/null | cut -d, -f4-15 | sed 's/,/\n/g'
}

extract 440 "$reference" | cat -n > "$refchroma"

sox "$other" -r 44100 -c 1 b.wav

for cents in $(seq -400 10 400) ; do

    hz=$(perl -e "print 440.0 * (2.0 ** ($cents / 1200.0))")

    extract "$hz" b.wav | cat -n > "$otherchroma"
    (
	echo "6 k 0"
	join "$refchroma" "$otherchroma" | \
	    awk '{ print $2, $3, "- d * +" }' # square difference
	echo "p"
    ) > "$tmpscript"

    dist=$( cat "$tmpscript" | dc )
    echo "$dist $hz"

done | tee "$scores"

echo
echo Scores:
sort -n "$scores"

echo
bestfreq=$(sort -n "$scores" | head -1 | awk '{ print $2; }')
echo "I reckon the tuning frequency is $bestfreq"


