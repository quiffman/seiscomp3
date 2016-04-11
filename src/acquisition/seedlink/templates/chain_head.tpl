<?xml version="1.0" standalone="yes"?>

<!-- Generated at #date# - Do not edit! -->
<!-- template: #template# -->

<chain
#SLNK#  timetable_loader="#pkgroot#/bin/load_timetable 127.0.0.1:#localport#"
#SLNK#  overlap_removal="initial"
  multistation="yes"
  batchmode="yes"
  netto="300"
  netdly="10"
  standby="10"
  keepalive="0"
  seqsave="60">

#TRIG#  <extension
#TRIG#    name = "trigger"
#TRIG#    filter = "._#detec_locid#_#detec_stream#._D"
#TRIG#    cmd = "python #pkgroot#/python/trigger_ext.py"
#TRIG#    recv_timeout = "0"
#TRIG#    send_timeout = "60"
#TRIG#    start_retry = "60"
#TRIG#    shutdown_wait = "10"/>
#TRIG#
