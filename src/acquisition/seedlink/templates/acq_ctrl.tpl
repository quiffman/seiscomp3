#!/bin/bash

# Generated at #date# - Do not edit!
# template: #template#

CFGDIR="#pkgroot#/config"
LOGDIR="#pkgroot#/log"
PIDDIR="#pkgroot#/status"
ARCDIR="#sds_path#"

ACTION="$1"
shift

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
        ulimit -n $(ulimit -Hn) 2>/dev/null

#DIGS#        if trylock $PIDDIR/digiserv.pid; then
#DIGS#            echo "starting digiserv"
#DIGS#            digiserv -v \
#DIGS#              -f $CFGDIR/digiserv.ini \
#DIGS#              >> $LOGDIR/digiserv.log 2>&1 &
#DIGS#        else
#DIGS#            echo "digiserv is running"
#DIGS#        fi
#DIGS#
#DGSD#        if trylock $PIDDIR/digiserv.pid; then
#DGSD#            echo "starting digiserv"
#DGSD#            digiserv -D -v \
#DGSD#              -f $CFGDIR/digiserv.ini \
#DGSD#              >> $LOGDIR/digiserv.log 2>&1
#DGSD#        else
#DGSD#            echo "digiserv is running"
#DGSD#        fi
#DGSD#
#SLNK#        if trylock $PIDDIR/seedlink.pid; then
#SLNK#            echo "starting seedlink"
#SLNK#            seedlink -v \
#SLNK#              -f $CFGDIR/seedlink.ini \
#SLNK#              >> $LOGDIR/seedlink.log 2>&1 &
#SLNK#        else
#SLNK#            echo "seedlink is running"
#SLNK#        fi
#SLNK#
#SLKD#        if trylock $PIDDIR/seedlink.pid; then
#SLKD#            echo "starting seedlink"
#SLKD#            seedlink -D -v \
#SLKD#              -f $CFGDIR/seedlink.ini \
#SLKD#              >> $LOGDIR/seedlink.log 2>&1
#SLKD#        else
#SLKD#            echo "seedlink is running"
#SLKD#        fi
#SLKD#
        if [ "$(echo $CFGDIR/slarchive_*)" != "$CFGDIR/slarchive_*" ]; then
            for f in $CFGDIR/slarchive_*; do
                host="$(echo ${f##*/slarchive_} | cut -d '_' -f 1)"
                port="$(echo ${f##*/slarchive_} | cut -d '_' -f 2)"
                if trylock "$PIDDIR/slarchive_${host}_${port}.pid"; then
                    echo "starting client slarchive $host:$port"
                    run_with_lock "$PIDDIR/slarchive_${host}_${port}.pid" slarchive -b -x "$PIDDIR/slarchive_${host}_${port}.seq:10000" -SDS "$ARCDIR" -nt 900 -Fi:1 -Fc:900 -l "$f" $host:$port >>"$LOGDIR/slarchive_${host}_${port}.log" 2>&1 &
                else
                    echo "client slarchive $host:$port is running"
                fi
            done
        fi
        ;;
    stop)
        if [ "$(echo $CFGDIR/slarchive_*)" != "$CFGDIR/slarchive_*" ]; then
            for f in $CFGDIR/slarchive_*; do
                host="$(echo ${f##*/slarchive_} | cut -d '_' -f 1)"
                port="$(echo ${f##*/slarchive_} | cut -d '_' -f 2)"
                if trylock "$PIDDIR/slarchive_${host}_${port}.pid"; then
                    echo "client slarchive $host:$port is not running"
                else
                    echo "shutting down client slarchive $host:$port"
                    killwait 10 $PIDDIR/slarchive_${host}_${port}.pid >/dev/null 2>&1
                fi
            done
        fi
#SLNK#
#SLNK#        if [ $# -eq 0 ]; then
#SLNK#            if trylock $PIDDIR/seedlink.pid; then
#SLNK#                echo "seedlink is not running"
#SLNK#            else
#SLNK#                echo "shutting down seedlink"
#SLNK#                killwait 300 $PIDDIR/seedlink.pid >/dev/null 2>&1
#SLNK#            fi
#SLNK#            rm -f $PIDDIR/seedlink.pid
#SLNK#        fi
#SLKD#
#SLKD#        if [ $# -eq 0 ]; then
#SLKD#            if trylock $PIDDIR/seedlink.pid; then
#SLKD#                echo "seedlink is not running"
#SLKD#            else
#SLKD#                echo "shutting down seedlink"
#SLKD#                killwait 300 $PIDDIR/seedlink.pid >/dev/null 2>&1
#SLKD#            fi
#SLKD#            rm -f $PIDDIR/seedlink.pid
#SLKD#        fi
#DIGS#
#DIGS#        if [ $# -eq 0 ]; then
#DIGS#            if trylock $PIDDIR/digiserv.pid; then
#DIGS#                echo "digiserv is not running"
#DIGS#            else
#DIGS#                echo "shutting down digiserv"
#DIGS#                killwait 300 $PIDDIR/digiserv.pid >/dev/null 2>&1
#DIGS#            fi
#DIGS#            rm -f $PIDDIR/digiserv.pid
#DIGS#        fi
#DGSD#
#DGSD#        if [ $# -eq 0 ]; then
#DGSD#            if trylock $PIDDIR/digiserv.pid; then
#DGSD#                echo "digiserv is not running"
#DGSD#            else
#DGSD#                echo "shutting down digiserv"
#DGSD#                killwait 300 $PIDDIR/digiserv.pid >/dev/null 2>&1
#DGSD#            fi
#DGSD#            rm -f $PIDDIR/digiserv.pid
#DGSD#        fi
        ;;
    *)
        echo "Usage: $0 {start|stop|check}"
        exit 1
esac

