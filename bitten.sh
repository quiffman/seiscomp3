#!/bin/bash
#bitten-slave -f bitten.conf --dry-run --verbose --keep-files --work-dir=. --build-dir="build" http://st110.gfz-potsdam.de/seiscomp/builds
bitten-slave -f bitten.conf --verbose --keep-files --work-dir=. --build-dir="build" http://st110.gfz-potsdam.de/seiscomp/builds
