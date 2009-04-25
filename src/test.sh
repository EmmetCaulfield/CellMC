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
    ps1)
	suffix='clr'
	flags='-M'
	;;
    cell2)
	suffix='ibm'
	flags=''
	;;
    *)
	echo 'Unknown machine'
	exit 1
	;;
esac

dir="results.$suffix"
mkdir $dir || errexit "$dir already exists"


dd4=(  'dd4' '../data/sbml/models/dd.xml'   '10000'    '10' )
dd5=(  'dd5' 'dd5.xml'                      '10000'    '10' )
me=(   'me'  'me-opt.xml'                 '1000000'  '1000' )
cr=(   'cr'  'cr-opt.xml'                   '10000'    '25' )
hsrA=( 'hsr' 'hsr-opt.xml'		    '10000'    '50' )
hsrB=( 'hsr' 'hsr-opt.xml'                   '1000'   '500' )

prg=('dd4' 'dd5' 'me' 'cr' 'hsrA' 'hsrB')


for p in ${prg[*]}; do
    eval x=\${$p[0]}.$suffix
    eval f=\${$p[1]}
    ./cellmc $flags -o $x $f
done

for ((i=1;i<=10;i++)); do
    for p in ${prg[*]}; do
	eval x=\${$p[0]}.$suffix
	eval n=\${$p[2]}
	eval t=\${$p[3]}
	./$x -o $dir/$x-${n}x$t-$i.out $n $t
    done
done
