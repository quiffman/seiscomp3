#!/bin/bash

# Generated at #date# - Do not edit!
# template: #template#

export SEISCOMP_ROOT="#seiscomp_root#"

if [ ! -r "$SEISCOMP_ROOT/lib/env.sh" ]; then
    echo "Cannot read $SEISCOMP_ROOT/lib/env.sh"
    exit 1
fi

source "$SEISCOMP_ROOT/lib/env.sh"

exec "#pkgroot#/bin/scqc" -s -l "#pkgroot#/status/scqc.pid" --archive --auto-time "$@"

