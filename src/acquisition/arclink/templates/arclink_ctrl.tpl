#!/bin/bash

# Generated at #date# - Do not edit!
# template: #template#

CFGDIR="#pkgroot#/config"
LOGDIR="#pkgroot#/log"
PIDDIR="#pkgroot#/status"

ACTION="$1"
trap '' SIGHUP

killwait() {
    kill $(cat "$2")
    if ! waitlock $1 "$2"; then
        echo "timeout exceeded"
        kill -9 $(cat "$2")
        return 1
    fi

    return 0
}

case "$ACTION" in
    start | check)
        if trylock $PIDDIR/arclink.pid; then
            echo "starting arclink"
#SY#        arclink -D \
#NS#        arclink \
              -f $CFGDIR/arclink.ini \
              >> $LOGDIR/arclink.log 2>&1 &
        else
            echo "arclink is running"
        fi
        ;;
    stop)
        if trylock $PIDDIR/arclink.pid; then
            echo "arclink is not running"
        else
            echo "shutting down arclink"
            killwait 30 $PIDDIR/arclink.pid >/dev/null 2>&1
        fi
        ;;
    *)
        echo "Usage: $0 {start|stop|check}"
        exit 1
esac

