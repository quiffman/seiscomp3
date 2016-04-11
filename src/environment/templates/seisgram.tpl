#!/bin/bash

# Generated at #date# - Do not edit!
# template: #template#

export SEISCOMP_ROOT="#seiscomp_root#"

eval java -cp $SEISCOMP_ROOT/seisgram/lib/SeisGram2K52.jar net.alomax.seisgram2k.SeisGram2K $(cat $SEISCOMP_ROOT/seisgram/config/seisgram.rc) -seedlink.groupchannels YES -commands.onread rmean -display.font=1.2,BOLD

