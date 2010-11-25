#!/bin/bash

workingdir=~/work/katcp-git

hostsfile=$workingdir/kcs/scripts/hosts
kcpcmd="$workingdir/cmd/kcpcmd -q -s localhost:7147 roach add"

grep roach.*x $hostsfile | tr -s \  | (while read myip myhost; do $kcpcmd $myhost $myip xport ; done)

grep 'roach[[:digit:]]*\.' $hostsfile | tr -s \ | (while read myip myhost; do $kcpcmd $myhost $myip spare ; done)