#!/bin/bash

function errexit () {
    echo $@
    exit 1
}


M=5

h=$(hostname)
suffix='unk'
case $(hostname) in
    scrat)
	suffix='amd'
	flags=''
	;;
    skara)
	suffix='ps3'
	flags=''
	;;
    pc-emca5039)
	suffix='duo'
	flags='-m'
	;;
    ps1.*)
	suffix='clr'
	flags='-M'
	;;
    cell2.*)
	suffix='ibm'
	flags=''
	;;
    grad1.*)
	suffix='grd'
	flags='-m'
	;;
    arich)
	suffix='opt'
	;;
    *)
	echo 'Unknown machine'
	exit 1
	;;
esac

dir="results.$suffix"
#mkdir $dir || errexit "$dir already exists"


dd=(  'dd'  '../data/sbml/models/dd.xml'   '1000'  '20' )
me=(  'me'  'me-opt.xml'  '10000' '500' )
cr=(  'cr'  'cr-opt.xml'   '1000'  '25' )
hsr=( 'hsr' 'hsr-opt.xml'  '1000'  '50' )

prg=('dd' 'me' 'cr' 'hsr')

for p in ${prg[*]}; do
    eval x=\${$p[0]}p.$suffix
    eval f=\${$p[1]}
    ./cellmc --no-valid -mpo $x $f
done

for ((i=1;i<=10;i++)); do
    for p in ${prg[*]}; do
	eval x=\${$p[0]}p.$suffix
	eval n=\${$p[2]}
	eval t=\${$p[3]}
	f="$dir/$x-c8-${n}x$t-$i.xsl"
	if ! [ -f $f ]; then
	    echo ./$x -c8 -o $f $n $t
	fi
    done
done
