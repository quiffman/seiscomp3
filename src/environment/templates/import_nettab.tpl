#!/bin/bash

# Generated at #date# - Do not edit!
# template: #template#

export SEISCOMP_ROOT="#seiscomp_root#"

if [ ! -r "$SEISCOMP_ROOT/lib/env.sh" ]; then
    echo "Cannot read $SEISCOMP_ROOT/lib/env.sh"
    exit 1
fi

source "$SEISCOMP_ROOT/lib/env.sh"

if [ ! -r "$SEISCOMP_ROOT/lib/keyutils.sh" ]; then
    echo "Cannot read $SEISCOMP_ROOT/lib/keyutils.sh"
    exit 1
fi

source "$SEISCOMP_ROOT/lib/keyutils.sh"

network_cfg="$SEISCOMP_ROOT/templates/network.cfg"
if [ ! -r "$network_cfg" ]; then
    echo "Cannot read $network_cfg!"
    exit 1
fi

station_cfg="$SEISCOMP_ROOT/templates/station.cfg"
if [ ! -r "$station_cfg" ]; then
    echo "Cannot read $station_cfg!"
    exit 1
fi

trunk_cfg="$SEISCOMP_ROOT/trunk/templates/station.cfg"
if [ ! -r "$trunk_cfg" ]; then
    echo "Cannot read $trunk_cfg!"
    exit 1
fi

if [ -d "$SEISCOMP_ROOT/autopick" ]; then
    autopick_cfg="$SEISCOMP_ROOT/autopick/templates/station.cfg"
    if [ ! -r "$autopick_cfg" ]; then
        echo "Cannot read $autopick_cfg!"
        exit 1
    fi
fi

KEY_VERSION="2.5"
DEFAULT_PACKAGES="acquisition:geofon"
DEFAULT_ORIENTATION="Z 0.0 -90.0; N 0.0 0.0; E 90.0 0.0"

usage="Usage: import_nettab [-p packages] <file>"
eval set -- "$(getopt -q -o p: -- "$@")"

while :; do
    case "$1" in
        -p) DEFAULT_PACKAGES="$2"; shift 2 ;;
        --) shift; break ;;
        *)  echo "$usage"; exit 1 ;;
    esac
done

if [ $# -ne 1 ]; then
    echo "$usage"
    exit 1
fi

INPUT_FILE="$1"

optionalFile() {
    test -e "$1" && echo "$1"
}

getGain() {
    awk -v device="$1" -v sn="$2" -v chan="$3" '/^[:space:]*[^[:space:]#]/{
        gsub("\\*", ".*", $2);
        gsub("\\*", ".*", $3);
        gsub("?", ".", $2);
        gsub("?", ".", $3);
        if ( $1 == device && match(sn, $2) && match(chan, $3) ) {
            print $4;
            exit;
        }
    }' "$SEISCOMP_ROOT/config/gain.tab" "$(optionalFile "$SEISCOMP_ROOT/config/gain.dlsv")"
}

mkdir -p "$SEISCOMP_ROOT/key" "$SEISCOMP_ROOT/trunk/key"

laststat=""
while read line; do
    echo $line | grep -q '^[:space:]*[^[:space:]#]' || continue

    set -- $line
    
    if [ -z "$6" ]; then
        NET="$3"
        source "$SEISCOMP_ROOT/key/network_${NET}" 2>/dev/null

        NET_DESC="$(echo $1 | sed -e 's/_/ /g')"
        NET_NAME="$2"
        
        echo "+ network $NET_DESC"
        
        OutputKeys $network_cfg >"$SEISCOMP_ROOT/key/network_${NET}"
        continue
    fi

    test -n "${12}" && continue

    if [ "$laststat" != "$1" ]; then
        STATION="$1"
        PACKAGES="$DEFAULT_PACKAGES"
        source "$SEISCOMP_ROOT/key/station_${NET}_${STATION}" 2>/dev/null
    
        STAT_DESC="$(echo $2 | sed -e 's/_/ /g')"
        DATALOGGER="$(echo $3 | cut -d '%' -f 1)"
        DATALOGGER_SN="$(echo $3 | cut -d '%' -f 2- | sed -e 's/%/\//g')"
        SEISMOMETER1="$(echo $4 | cut -d '%' -f 1)"
        SEISMOMETER_SN1="$(echo $4 | cut -d '%' -f 2- | sed -e 's/%/\//g')"
        GAIN_MULT1="$5"
        SAMPLING1="$6"
        LATITUDE="$7"
        LONGITUDE="$8"
        ELEVATION="$9"
        DEPTH1="${10}"
        START_DATE="$(echo ${11} | cut -d ':' -f 1)"
        test -z "$ORIENTATION1" && ORIENTATION1="$DEFAULT_ORIENTATION"
    
        DATALOGGER2=""
        DATALOGGER_SN2=""
        SEISMOMETER2=""
        SEISMOMETER_SN2=""
        GAIN_MULT2=""
        SAMPLING2=""
        DEPTH2=""
        ORIENTATION2=""
        CONFIGURED="yes"

        echo "  + station $STAT_DESC"

    else
        DATALOGGER2="$(echo $3 | cut -d '%' -f 1)"
        DATALOGGER_SN2="$(echo $3 | cut -d '%' -f 2)"
        SEISMOMETER2="$(echo $4 | cut -d '%' -f 1)"
        SEISMOMETER_SN2="$(echo $4 | cut -d '%' -f 2)"
        GAIN_MULT2="$5"
        SAMPLING2="$6"
        DEPTH2="${10}"
        test -z "$ORIENTATION2" && ORIENTATION2="$DEFAULT_ORIENTATION"
        
        if [ "$DATALOGGER2" = "$DATALOGGER" -a "$DATALOGGER_SN2" = "$DATALOGGER_SN" ]; then
            DATALOGGER2=""
            DATALOGGER_SN2=""
        fi
    fi

    OutputKeys $station_cfg >"$SEISCOMP_ROOT/key/station_${NET}_${STATION}"

    if [ "$laststat" != "$1" ]; then
        DETEC_STREAM="BH"
        DETEC_LOCID=""
        DETEC_FILTER="RMHP(10)>>ITAPER(30)>>BW(4,0.7,2)>>STALTA(2,80)"
        TRIG_ON="3"
        TRIG_OFF="1.5"
        source "$SEISCOMP_ROOT/trunk/key/station_${NET}_${STATION}" 2>/dev/null
        OutputKeys $trunk_cfg >"$SEISCOMP_ROOT/trunk/key/station_${NET}_${STATION}"

        if [ -d "$SEISCOMP_ROOT/autopick" ]; then
            DETEC_FILTER="bandpass 0.7 2.0 4"
            STA_LEN="2"
            LTA_LEN="100"
            source "$SEISCOMP_ROOT/autopick/key/station_${NET}_${STATION}" 2>/dev/null
                
            dl_gain="$(getGain "$DATALOGGER" "$DATALOGGER_SN" "${DETEC_STREAM}Z")"
            sm_gain="$(getGain "$SEISMOMETER1" "$SEISMOMETER_SN1" "${DETEC_STREAM}Z")"
            
            if [ -z "$dl_gain" ]; then
                echo "$DATALOGGER $DATALOGGER_SN ${DETEC_STREAM}Z not found in $SEISCOMP_ROOT/config/gain.tab"
            fi
            
            if [ -z "$sm_gain" ]; then
                echo "$SEISMOMETER1 $SEISMOMETER_SN1 ${DETEC_STREAM}Z not found in $SEISCOMP_ROOT/config/gain.tab"
            fi
            
            if [ -n "$dl_gain" -a -n "$sm_gain" ]; then
                GAIN="$(echo $dl_gain $sm_gain $GAIN_MULT1 | awk '{ print $1 * $2 * $3 / 10^9 }')"
                OutputKeys $autopick_cfg >"$SEISCOMP_ROOT/autopick/key/station_${NET}_${STATION}"
            fi
        fi
    fi

    laststat="$STATION"

done <"$INPUT_FILE"

