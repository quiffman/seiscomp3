#!/bin/bash

# Generated at #date# - Do not edit!
# template: #template#

KEEP=30

find #pkgroot#/seq-backup -type f -mtime +$KEEP -exec rm -f '{}' \;

cd #pkgroot#/status
tar cf - *.seq *.state | gzip >#pkgroot#/seq-backup/seq-`date +'%Y%m%d'`.tgz

