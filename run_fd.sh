#!/bin/sh
echo nickname: $1
shift
echo params = "$@"
#sleep 1
python2.7 ./fast-downward.py "$@"
#echo running python ~/fd/fast-downward.py "$@"
