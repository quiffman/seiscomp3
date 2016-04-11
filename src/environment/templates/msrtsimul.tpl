#!/bin/bash

# Generated at #date# - Do not edit!
# template: #template#

export SEISCOMP_ROOT="#seiscomp_root#"

if [ ! -r "$SEISCOMP_ROOT/lib/env.sh" ]; then
    echo "Cannot read $SEISCOMP_ROOT/lib/env.sh"
    exit 1
fi

source "$SEISCOMP_ROOT/lib/env.sh"

if [ ! -p "$SEISCOMP_ROOT/acquisition/mseedfifo" ]; then
    echo "Named pipe $SEISCOMP_ROOT/acquisition/mseedfifo does not exist"
    exit 1
fi

exec python "$SEISCOMP_ROOT/acquisition/lib/msrtsimul.py" "$@" >$SEISCOMP_ROOT/acquisition/mseedfifo

