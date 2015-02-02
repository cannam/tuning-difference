#!/bin/bash

##!!! This is unfinished -- abandoned because a premise turned out not to be sound

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
trap "rm $transform" 0

provisional=$(sonic-annotator -d vamp:nnls-chroma:tuning:tuning "$other" -w csv --csv-stdout | tail -1 | awk -F, '{ print $4 }')

ref_tonic=$(sonic-annotator -t "keydetector.ttl" "$reference" -w csv --csv-stdout --csv-omit-filename --summary mode|tail -1|awk -F, '{ print $4 }')

cat "keydetector.ttl" | sed "s,440,$provisional," > "$transform"

tonic=$(sonic-annotator -t "$transform" "$other" -w csv --csv-stdout --csv-omit-filename --summary mode|tail -1|awk -F, '{ print $4 }')

# modular arithmetic here is wrong. but it turns out our tuning freq
# estimation above is also wrong anyway for our test file

sum="print ($provisional * (2.0 ** ((($tonic - $ref_tonic) % 12) / 12.0)))"
freq=$(perl -e "$sum")

echo "Sum is: $sum"
echo "Estimated frequency is $freq (provisional: $provisional; reference tonic: $ref_tonic; test tonic: $tonic)"

