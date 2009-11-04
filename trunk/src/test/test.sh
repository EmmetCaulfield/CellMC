#!/bin/bash

# @file		test.sh
# @brief	Rudimentary test-harness
# @author	Emmet Caulfield
# $Rev$
#
# Rudimentary, but better than nothing, which is what we had before.
#
# $Id$

octave=octave
xsltp=xsltproc
cellmc=../cellmc
mdldir=../../data/sbml/models
tolpc=2
models=('dd' 'me' 'cr' 'hsr')

# Profiling run arguments:
pdd=(  '8'  '15' )
pme=(  '8' '500' )
pcr=(  '4'  '25' )
phsr=( '8'  '50' )

# Test run parameters
# -------------------
# Command-line arguments and expected value of trajectory length.
#
# I'm only confident of the "correct" expected value of the length of
# trajectories for 500s HSR and ME (roughly 3 Tr/s) is only
#
#    #trjs t_final   E(Rx/Tr)
tdd=(  '8'    '15'     '28000')
tme=(  '8'  '1000'      '3100')
tcr=(  '8'    '50' '274000000')
thsr=( '8'   '500'  '61000000')


function errexit () {
    code = "$1"; shift
    msg  = "$1"; shift
    if [ -z "$code" ]; then
	code=1
    fi
    if [ -z "$msg" ]; then
	msg='Unspecified error'
    fi
    echo "FATAL: $msg." >&2
    exit $code
}

function warn() {
    echo "WARNING: $@." >&2
}

# Make sure a model exists.
function model_exists () {
    m=$1; shift;
    sbml="$mdldir/$m.xml"
    if [ -e "$sbml" -a -r "$sbml" ]; then
	return 0;
    fi
    warn "Model '$m' does not exist"
    return 1;
}

function build_profiling_exe () {
    model=$1; shift;
    sbml="$mdldir/$model.xml"
    flags=$1; shift;

    echo -n "Building profiling executable for $model... "

    if $cellmc -po p$model $flags $sbml; then
	echo done.
    else
	echo FAILED.
	errexit 2 "Failed to build profiling executable for $model"
    fi
}

function build_exe () {
    model=$1; shift;
    sbml="$model-opt.xml"
    flags=$1; shift;

    echo -n "Building executable for $model... "

    if $cellmc -mo $model $flags $sbml; then
	echo done.
    else
	echo FAILED.
	errexit 2 "Failed to build executable for $model"
    fi
}

function profiling_run () {
    m="$1"; shift;

    echo -n "Profiling $m... "

    eval x=p$m
    eval n=\${p$m[0]}
    eval t=\${p$m[1]}
    rm -f $x.xsl
    if ./$x -o $x.xsl $n $t; then
	echo done.
    else
	echo FAILED.
	errexit 2 "$x failed"
    fi
}

function test_run () {
    m="$1"; shift;

    echo -n "Testing $m... "

    eval n=\${t$m[0]}
    eval t=\${t$m[1]}
    rm -f $m.out
    if ./$m -o $m.out $n $t; then
	echo done.
    else
	echo FAILED.
	errexit 2 "Test run of $m failed"
    fi
}

function apply_xslt () {
    model=$1; shift;
    flags=$1; shift;

    echo -n "Transforming model $model... "

    min="$mdldir/$model.xml"
    mout="$model-opt.xml"
    xslf="p$model.xsl"
    rm -f $mout
    if $xsltp -o $mout $xslf $min; then
	echo done.
    else
	echo FAILED.
	errexit 3 "Failed to transform $min-[$xslf]->$mout"
    fi
}


function extract_prof_data () {
    stub=p$1

    echo -n "Extracting metadata from $stub.xsl... "
    awk -F'[^0-9]+' '/RpT/ {print $2, 0}' $stub.xsl > $stub.cnt
    awk -F'[^0-9]+' '/<!-- # / {print $2, $3}' $stub.xsl >> $stub.cnt
    awk -F'[^0-9]+' '/select="s:reaction\[/ {print $2}' $stub.xsl > $stub.ord
    echo done.
}


function are_pops_nonnegative () {
    m="$1"; shift;

    echo -n "Checking '$m' final populations are nonnegative... "
    cat<<EOF | $octave -q
p=load("$m.out");
if min(min(p)')<0,
    exit(1);
end
EOF
    if [ $? -eq 0 ]; then
	echo passed.
    else
	echo FAILED.
    fi
}

function check_traj_length () {
    m="$1"; shift;

    echo -n "Checking trajectory length for '$m'... "
    n=$(awk -F= '/RpT/ {print $2}' $m.out)
    eval ex=\${t$m[2]}

    cat<<EOF | $octave -q
if $n<((1-$tolpc/100)*$ex),
    exit(1);
elseif $n>((1+$tolpc/100)*$ex),
    exit(2);
end
EOF
    rc=$?
    if [ $rc -eq 0 ]; then
	echo passed "($n ~ $ex)"
    else
	echo -n "FAILED "
	if [ $rc -eq 1 ]; then
	    echo "($n << $ex)"
	elif [ $rc -eq 2 ]; then
	    echo "($n >> $ex)"
	fi
    fi
}



#============================================================
# Body starts here
#------------------------------------------------------------

# Check for cellmc executable:
if [ ! -e $cellmc ]; then
    errexit 1 'cellmc not found'
elif [ ! -x $cellmc ]; then
    errexit 1 'cellmc not executable'
fi

# Check for command-line xslt processor:
if ! which $xsltp >/dev/null 2>&1; then
    errexit 1 "$xsltp required but not found"
fi

# Check for GNU octave:
if ! which $octave >/dev/null 2>&1; then
    errexit 1 "GNU Octave required but not found"
fi

# Iterate over the models, optimize and run them:
for m in $@; do
    if ! model_exists $m; then
	continue;
    fi
    build_profiling_exe $m
    profiling_run $m
    extract_prof_data $m 
    apply_xslt $m
    build_exe $m
    test_run $m
    are_pops_nonnegative $m
    check_traj_length $m
done
