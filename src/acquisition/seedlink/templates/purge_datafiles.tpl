#!/bin/bash

# Generated at #date# - Do not edit!
# template: #template#

CFGDIR="#pkgroot#/config"
ARCHIVE="#sds_path#"

for rc in $CFGDIR/rc_*; do
    station=${rc##*_}
    source $rc
    find "$ARCHIVE"/*/"$NET/$STATION" -type f -follow -mtime +$ARCH_KEEP -exec rm -f '{}' \;
done

