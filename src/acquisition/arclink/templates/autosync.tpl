#!/bin/bash

# Generated at #date# - Do not edit!
# template: #template#

export SEISCOMP_ROOT="#seiscomp_root#"

if [ ! -r "$SEISCOMP_ROOT/lib/env.sh" ]; then
    echo "Cannot read $SEISCOMP_ROOT/lib/env.sh"
    exit 1
fi

source "$SEISCOMP_ROOT/lib/env.sh"

usage="Usage: sync_db [-f] [-c]"
eval set -- "$(getopt -q -o fs -- "$@")"

fullsync=false
syslog=false

while :; do
    case "$1" in
        -f) fullsync=true; shift ;;
        -s) syslog=true; shift ;;
        --) shift; break ;;
        *)  echo "$usage"; exit 1 ;;
    esac
done

if [ $# -ne 0 ]; then
    echo "$usage"
    exit 1
fi

if $fullsync; then
    modified_after=""
else
    modified_after="$(cat "$SEISCOMP_ROOT/arclink/status/last_dbsync")"
fi

if $syslog; then
    extraflags="--syslog"
else
    extraflags="--console 1"
fi

TZ=GMT date --date '25 hours ago' +'%Y-%m-%dT%H:%M:%S.0Z' >"$SEISCOMP_ROOT/arclink/status/last_dbsync"

exec python "$SEISCOMP_ROOT/arclink/lib/sync_db.py" --source "#master_node#" --modified-after "$modified_after" $extraflags

kill `ps axu | grep reqhandler | grep -v grep | awk '{print$2}'`

