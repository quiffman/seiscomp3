#!/bin/sh

# Generated at #date# - Do not edit!
# template: #template#

export SEISCOMP_ROOT="#seiscomp_root#"

if [ $# -ne 1 ]; then
    echo "Usage: qplot <station_id>"
    exit 1
fi

STATID="$1"

if [ ! -e "$SEISCOMP_ROOT/qplot/config/rc_$STATID" ]; then
    echo "Station $STATID not found"
    exit 1
fi

source "$SEISCOMP_ROOT/qplot/config/rc_$STATID"
$SEISCOMP_ROOT/qplot/bin/slqplot -v -c xslqplot -f $SEISCOMP_ROOT/qplot/config/slqplot_${STATID} -F $SEISCOMP_ROOT/qplot/config/slqplot.coef -S ${NET}_${STATION} ${SRCHOST}:${SRCPORT}

