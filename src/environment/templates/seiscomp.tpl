#!/bin/bash

# Generated at #date# - Do not edit!
# template: #template#

export SEISCOMP_ROOT="#seiscomp_root#"

if [ "$SEISCOMP_LOCK" != true ]; then
    mkdir -p "$SEISCOMP_ROOT/status"
    while :; do
        SEISCOMP_LOCK=true $SEISCOMP_ROOT/bin/run_with_lock \
            $SEISCOMP_ROOT/status/seiscomp.pid bash -c "$0 $*"
        if [ $? -ne 255 ]; then exit $?; fi
        echo "...locked...trying again..."
        sleep 1
    done
fi

export PATH="$SEISCOMP_ROOT/bin:$PATH"

if ! cd "$SEISCOMP_ROOT"; then
    echo "Cannot change directory to $SEISCOMP_ROOT"
    exit 1
fi

if [ ! -r "lib/env.sh" ]; then
    echo "Cannot read $SEISCOMP_ROOT/lib/env.sh"
    exit 1
fi

source "lib/env.sh"

if [ ! -r "lib/keyutils.sh" ]; then
    echo "Cannot read $SEISCOMP_ROOT/lib/keyutils.sh"
    exit 1
fi

source "lib/keyutils.sh"

global_cfg="templates/global.cfg"
if [ ! -r "$global_cfg" ]; then
    echo "Cannot read $SEISCOMP_ROOT/$global_cfg!"
    exit 1
fi

network_cfg="templates/network.cfg"
if [ ! -r "$network_cfg" ]; then
    echo "Cannot read $SEISCOMP_ROOT/$network_cfg!"
    exit 1
fi

station_cfg="templates/station.cfg"
if [ ! -r "$station_cfg" ]; then
    echo "Cannot read $SEISCOMP_ROOT/$station_cfg!"
    exit 1
fi

VERSION="3.0 (2011.290)"
KEY_VERSION="2.5"

SED=sed
gsed -e p </dev/null >/dev/null 2>&1 && SED=gsed

grepq() {
    grep "$@" >/dev/null
}

_get_pkgmods() {
    pkgmods="$(for p in pkg/[0-9]*; do
        attr=$($p get_attributes)
        if ([ -z "$1" ] || echo $attr | grepq "\<$1\>") &&
            ([ "$CONFIG_STYLE" != simple ] || echo $attr | grep -vq "\<advanced\>"); then
            echo "${p##*/}"
        fi
    done)"
}

_init_globals() {
    ORGANIZATION=""
    CONTACT_EMAIL=""
    SDS_PATH="$SEISCOMP_ROOT/acquisition/archive"
    CONFIG_STYLE="advanced"
    SYSLOG="yes"
    SEEDLINK_PORT="18000"
    ARCLINK_PORT="18001"
    DIGISERV_PORT="60999"

    if [ -f "key/global" ]; then
        source key/global
    fi
    
    echo "0) Simple acquisition system"
    echo "1) Advanced acquisition and processing system"
    echo "2) ArcLink system"

    case "$CONFIG_STYLE" in
        simple) CHOICE=0 ;;
        arclink) CHOICE=2 ;;
        *) CHOICE=1 ;;
    esac

    while :; do
        Ask CHOICE "Configuration style" "$CHOICE"
        case "$CHOICE" in
            0) CONFIG_STYLE="simple"; break ;;
            1) CONFIG_STYLE="advanced"; break ;;
            2) CONFIG_STYLE="arclink"; break ;;
            *) echo "invalid input" ;;
        esac
    done
    
    Ask ORGANIZATION "Name of Data Center" "$ORGANIZATION"
    Ask CONTACT_EMAIL "Email of the responssible of the datacenter" "$CONTACT_EMAIL"
    Ask SDS_PATH "Path to waveform archive" "$SDS_PATH"

    if [ "$CONFIG_STYLE" != simple ]; then
        AskYN SYSLOG "Use syslog when supported" "$SYSLOG"
    else
        SYSLOG="no"
    fi

    OutputKeys $global_cfg >key/global
}

_edit_globals() {
    _init_globals
    _get_pkgmods globals
    
    pkglist="$(for p in $pkgmods; do echo -n "${p#*_} "; done)"
    echo
    echo "Following packages are selectable: $pkglist"

    while :; do
        Ask pkg "Select package"
        if [ -z "$pkg" ]; then
            return
        elif echo "$pkglist" | grepq "\<$pkg\>"; then
            break
        fi

        echo "Package $pkg not installed"
    done

    for p in $pkgmods; do
        if [ "$pkg" = "${p#*_}" ]; then
            pkg/$p edit_globals $profile
            return
        fi
    done

    echo "Cannot find $pkg config hook"
}

_edit_station() {
    _get_pkgmods station

    if [ "$(echo key/station_${NET}_*)" != "key/station_${NET}_*" ]; then
        echo -n "Following stations are defined in network ${NET}: "
        for k in key/station_${NET}_*; do
            echo -n "${k##*_} "
        done
        echo
    else
        echo "No stations defined in network ${NET}"
    fi

    Ask STATION "Station code"
    STATION="$(echo ${STATION} | tr '[:lower:]' '[:upper:]' | 
        sed -e 's/^[^A-Z0-9]*\([A-Z0-9]\{1,5\}\).*/\1/g')"

    STAT_DESC="$NET station"
    LATITUDE=""
    LONGITUDE=""
    ELEVATION=""
    DATALOGGER="Q330"
    DATALOGGER_SN="xxxx"
    SEISMOMETER1="STS-2N"
    SEISMOMETER_SN1="yyyy"
    UNIT1="M/S"
    GAIN_MULT1="1.0"
    SAMPLING1="100/20/1/0.1"
    ORIENTATION1='Z 0 -90; N 0 0; E 90 0'
    DEPTH1=""
    SEISMOMETER2=""
    SEISMOMETER_SN2=""
    UNIT2="M/S**2"
    GAIN_MULT2=""
    SAMPLING2=""
    ORIENTATION2='Z 0 -90; N 0 0; E 90 0'
    DEPTH2=""
    START_DATE="2006/001"
    PACKAGES="$(for p in $pkgmods; do echo -n "${p#*_} "; done)"
    CONFIGURED="no"

    if [ -z "$STATION" ]; then
        return
    elif [ -f "key/station_${NET}_${STATION}" ]; then
        echo
        echo "Editing existing station ${STATION}"
        source "key/station_${NET}_${STATION}"
    else
        echo
        echo "Adding new station ${STATION}"
    fi
 
    Ask STAT_DESC "Station description" "$STAT_DESC"

    if [ "$CONFIG_STYLE" != simple ]; then
        Ask LATITUDE "Latitude" "$LATITUDE"
        Ask LONGITUDE "Longitude" "$LONGITUDE"
        Ask ELEVATION "Elevation" "$ELEVATION"
        Ask DATALOGGER "Datalogger" "$DATALOGGER"
        Ask SEISMOMETER1 "Primary sensor" "$SEISMOMETER1"
        Ask UNIT1 "Unit of $SEISMOMETER1" "$UNIT1"
        Ask GAIN_MULT1 "Gain multiplier of $SEISMOMETER1" "$GAIN_MULT1"
        Ask SAMPLING1 "Sample rates of $SEISMOMETER1" "$SAMPLING1"
        Ask DEPTH1 "Depth of $SEISMOMETER1" "$DEPTH1"
        
        Ask SEISMOMETER2 "Secondary sensor (eg., strong-motion)" "$SEISMOMETER2"
        if [ -n "$SEISMOMETER2" ]; then
            Ask UNIT2 "Unit of $SEISMOMETER2" "$UNIT2"
            Ask GAIN_MULT2 "Gain multiplier of $SEISMOMETER2" "$GAIN_MULT2"
            Ask SAMPLING2 "Sample rates of $SEISMOMETER2" "$SAMPLING2"
            Ask DEPTH2 "Depth of $SEISMOMETER2" "$DEPTH2"
        fi
        
        Ask START_DATE "Start date" "$START_DATE"
        CONFIGURED="yes"
    fi

    pkgs="$PACKAGES"
    PACKAGES=""
    for p in $pkgmods; do
        pkg="${p#*_}"
        if echo "$pkgs" | grepq "\<$pkg\>"; then
            enabled="yes"
            profile="$(echo -n "$pkgs" | sed -e "s/.*\<$pkg\>\(:\([^ ]*\)\)\?.*/\2/")"
        else
            enabled="no"
            profile=""
        fi

        AskYN enabled "Enable $pkg for ${NET}_${STATION}" "$enabled"
        if [ "$enabled" = yes ]; then
            if [ -z "$profile" ]; then
                useprof="no"
            else
                useprof="yes"
            fi
            
            if [ "$(echo $pkg/key/profile_*)" != "$pkg/key/profile_*" ]; then
                AskYN useprof "Use predefined profile" "$useprof"
                if [ "$useprof" = yes ]; then
                    echo -n "Following profiles are defined for package $pkg: "
                    for pf in $pkg/key/profile_*; do
                        echo -n "${pf##*/profile_} "
                    done
                    echo

                    while :; do
                        Ask profile "Select profile" "$profile"
                        if [ -f "$pkg/key/profile_${profile}" ]; then
                            break
                        fi

                        echo "Profile $profile not found"
                    done
                fi
            fi

            if [ "$useprof" = no ]; then
                pkg/$p edit_station $NET $STATION
            fi
        fi

        if [ "$enabled" = yes ]; then
            if [ -n "$PACKAGES" ]; then
                PACKAGES="$PACKAGES "
            fi
            if [ "$useprof" = yes ]; then
                PACKAGES="${PACKAGES}$pkg:$profile"
            else
                PACKAGES="${PACKAGES}$pkg"
            fi
        fi
    done
    
    OutputKeys "$station_cfg" >key/station_${NET}_${STATION}
}                

_remove_station() {
    if [ "$(echo key/station_${NET}_*)" != "key/station_${NET}_*" ]; then
        echo -n "Following stations are defined in network ${NET}: "
        for k in key/station_${NET}_*; do
            echo -n "${k##*_} "
        done
        echo
        Ask STATION "Station to remove"
        STATION="$(echo ${STATION} | tr '[:lower:]' '[:upper:]' | 
            sed -e 's/^[^A-Z0-9]*\([A-Z0-9]\{1,5\}\).*/\1/g')"

        if [ -f "key/station_${NET}_${STATION}" ]; then
            echo "Removing ${NET}_${STATION}"
            rm -f key/station_${NET}_${STATION}
        else
            echo "Station ${NET}_${STATION} not found"
        fi
    else
        echo "No stations defined in network ${NET}"
    fi
}

_edit_network() {
    if [ "$(echo key/network_*)" != "key/network_*" ]; then
        echo -n "Following networks are defined: "
        for k in key/network_*; do
            echo -n "${k##*_} "
        done
        echo
    else
        echo "No networks defined"
    fi
 
    Ask NET "Network code"
    NET="$(echo ${NET} | tr '[:lower:]' '[:upper:]' | 
        sed -e 's/^[^A-Z0-9]*\([A-Z0-9]\{1,2\}\).*/\1/g')"

    NET_DESC="$NET network"
    NET_NAME="${NET}-Net"

    if [ -z "$NET" ]; then
        return
    elif [ -f "key/network_${NET}" ]; then
        echo
        echo "Editing existing network ${NET}"
        source "key/network_${NET}"
    else
        echo
        echo "Adding new network ${NET}"
    fi
    
    Ask NET_DESC "Network description" "$NET_DESC"
    Ask NET_NAME "Network name" "$NET_NAME"
    OutputKeys "$network_cfg" >key/network_$NET

    while :; do
        echo
        echo "A) Add/Edit station"
        echo "R) Remove station"
        echo "Q) Back to main menu"
        Ask reply "Command?" "A"
        echo

        case "$reply" in
            A|a) _edit_station ;;
            R|r) _remove_station ;;
            Q|q) return ;;
            *)   echo "invalid input" ;;
        esac
    done
}

_remove_network() {
    if [ "$(echo key/network_*)" != "key/network_*" ]; then
        echo -n "Following networks are defined: "
        for k in key/network_*; do
            echo -n "${k##*_} "
        done
        echo
        Ask NET "Network to remove"
        NET="$(echo $NET | tr '[:lower:]' '[:upper:]' | 
            sed -e 's/^[^A-Z0-9]*\([A-Z0-9]\{1,2\}\).*/\1/g')"

        if [ -f "key/network_$NET" ]; then
            echo "Removing ${NET}"
            rm -f key/network_${NET}
            rm -f key/station_${NET}_*
        else
            echo "Network ${NET} not found"
        fi
    else
        echo "No networks defined"
    fi
} 

_edit_profile() {
    _get_pkgmods profile

    pkglist="$(for p in $pkgmods; do echo -n "${p#*_} "; done)"
    echo "Following packages are selectable: $pkglist"

    while :; do
        Ask pkg "Select package"
        if [ -z "$pkg" ]; then
            return
        elif echo "$pkglist" | grepq "\<$pkg\>"; then
            break
        fi

        echo "Package $pkg not installed"
    done

    if [ "$(echo $pkg/key/profile_*)" != "$pkg/key/profile_*" ]; then
        echo -n "Following profiles are defined for package $pkg: "
        for pf in $pkg/key/profile_*; do
            echo -n "${pf##*/profile_} "
        done
        echo
    else
        echo "No profiles defined for package $pkg"
    fi

    Ask profile "Select existing or new profile"
    profile="$(echo $profile | sed -e 's/[[:space:]]/_/g')"
    if [ -z "$profile" ]; then
        return
    elif [ -f "$pkg/key/profile_$profile" ]; then
        echo
        echo "Editing existing profile $profile"
    else
        echo
        echo "Creating new profile $profile"
    fi

    for p in $pkgmods; do
        if [ "$pkg" = "${p#*_}" ]; then
            pkg/$p edit_profile $profile
            return
        fi
    done

    echo "Cannot find $pkg config hook"
}

_write_conf() {
    LoadConfig "$global_cfg" "$network_cfg" "$station_cfg"

    echo "Writing global configuration"
    
    if [ ! -f "key/global" ]; then
        echo "Cannot find $SEISCOMP_ROOT/key/global"
        return
    fi

    source key/global
    
    ORGANIZATION="$(echo $ORGANIZATION | sed -e 's/[[:space:]]/_/g')"
    OutputFile templates/nettab_head.tpl >config/net.tab

    for n in key/network_*; do
        if [ "$n" = "key/network_*" ]; then
            echo "No networks defined"
            break
        fi
        
        NET="${n##*_}"
        source $n
        NET_DESC="$(echo $NET_DESC | sed -e 's/[[:space:]]/_/g')"
        NET_NAME="$(echo $NET_NAME | sed -e 's/[[:space:]]/_/g')"
        OutputFile templates/nettab_network.tpl >>config/net.tab
        for s in key/station_${NET}_*; do
            if [ "$s" = "key/station_${NET}_*" ]; then
                echo "No stations defined for network $NET"
                break
            fi

            STATION="${s##*_}"
            source $s
            if [ "$CONFIGURED" = yes ]; then
                STAT_DESC="$(echo $STAT_DESC | sed -e 's/[[:space:]]/_/g')"
                SEISMOMETER="$SEISMOMETER1"
                SEISMOMETER_SN="$SEISMOMETER_SN1"
                GAIN_MULT="$GAIN_MULT1"
                SAMPLING="$SAMPLING1"
                DEPTH="$DEPTH1"
                OutputFile templates/nettab_station.tpl >>config/net.tab
                if [ -n "$SEISMOMETER2" ]; then
                    test -n "$DATALOGGER2" && DATALOGGER="$DATALOGGER2"
                    test -n "$DATALOGGER_SN2" && DATALOGGER="$DATALOGGER_SN2"
                    SEISMOMETER="$SEISMOMETER2"
                    SEISMOMETER_SN="$SEISMOMETER_SN2"
                    GAIN_MULT="$GAIN_MULT2"
                    SAMPLING="$SAMPLING2"
                    DEPTH="$DEPTH2"
                    OutputFile templates/nettab_station.tpl >>config/net.tab
                fi
            fi
        done
    done

    _get_pkgmods
    
    for p in $pkgmods; do
        pkg="${p#*_}"
        echo
        echo "Writing configuration for $pkg"
        pkg/$p write_conf
    done
}

_configure() {
    mkdir -p key

    if [ ! -f "key/global" -o "$CONFIG_STYLE" = simple ]; then
        echo "Initializing global parameters"
        _init_globals
    fi
    
    _get_pkgmods globals

    for p in $pkgmods; do
        pkg="${p#*_}"
        if [ ! -f "$pkg/key/global" -o "$CONFIG_STYLE" = simple ]; then
            echo "Initializing $pkg"
            pkg/$p edit_globals
        fi
    done

    if [ "$CONFIG_STYLE" = simple ]; then
        while :; do
            echo
            echo "A) Add/Edit network"
            echo "R) Remove network"
            echo "W) Write configuration and quit"
            echo "Q) Quit without writing configuration"
            Ask reply "Command?" "A"
            echo

            case "$reply" in
                A|a) _edit_network ;;
                R|r) _remove_network ;;
                W|w) _write_conf
                     exit 0 ;;
                Q|q) exit 0 ;;
                *)   echo "invalid input" ;;
            esac
        done
    else
        while :; do
            echo
            echo "G) Edit global parameters"
            echo "A) Add/Edit network"
            echo "R) Remove network"
            echo "P) Add/Edit configuration profile"
            echo "W) Write configuration and quit"
            echo "Q) Quit without writing configuration"
            Ask reply "Command?" "A"
            echo

            case "$reply" in
                G|g) _edit_globals ;;
                A|a) _edit_network ;;
                R|r) _remove_network ;;
                P|p) _edit_profile ;;
                W|w) _write_conf
                     exit 0 ;;
                Q|q) exit 0 ;;
                *)   echo "invalid input" ;;
            esac
        done
    fi
}

_start() {
    for p in $pkgmods; do
        pkg="${p#*_}"
        if [ -z "$*" ] || echo "$*" | grepq "\<$pkg\>"; then
            touch "status/$pkg.run"
            pkg/$p start </dev/null
        fi
    done
}

_stop() {
    for p in $(echo "$pkgmods" | sort -r); do
        pkg="${p#*_}"
        if [ -z "$*" ] || echo "$*" | grepq "\<$pkg\>"; then
            pkg/$p stop </dev/null
            rm -f "status/$pkg.run"
        fi
    done
}

echo "SeisComP version $VERSION" 1>&2

if [ "$#" -eq 0 ]; then
    echo "Usage: seiscomp {{start|stop|restart|check|print_crontab} [pkg]}|config" 1>&2
    exit 0
fi

action="$1"
shift

ORGANIZATION=""
CONFIG_STYLE="advanced"
SYSLOG="no"

if [ -f "key/global" ]; then
    source key/global
fi

_get_pkgmods

case "$action" in
    start)
        _start $*
        exit 0
        ;;
    stop)
        _stop $*
        exit 0
        ;;
    restart)
        _stop $*
        _start $*
        exit 0
        ;;
    check)
        for p in $pkgmods; do
            pkg="${p#*_}"
            if [ -f "status/$pkg.run" ]; then
                if [ -z "$*" ] || echo "$*" | grepq "\<$pkg\>"; then
                    touch "status/$pkg.run"
                    pkg/$p check </dev/null
                fi
            fi
        done
        exit 0
        ;;
    print_crontab)
        if [ $# -eq 0 ]; then
            echo "*/3 * * * * $SEISCOMP_ROOT/bin/seiscomp check >/dev/null 2>&1"
            for p in $pkgmods; do
                pkg/$p print_crontab
            done
            exit 0
        fi
        ;;
    config)
        if [ $# -eq 0 ]; then
            _configure
            exit 0
        fi
        ;;
esac

echo "Invalid parameters" 1>&2
exit 1

