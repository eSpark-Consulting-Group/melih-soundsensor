#!/bin/bash
# set wifi SSID & password on Atom32

function usage()
{
    cat << HEREDOC
    Usage: $progname [-h|--help] [-n|--name NAME] [-p|--password PASS]
HEREDOC
}

progname=$(basename $0)

OPTS=$(getopt -o "hn:p:" --long "help,name:,password:" -n "$progname" -- "$@")
if [ $? != 0 ] ; then echo "Error in command line arguments." >&2 ; usage; exit 1 ; fi
eval set -- "$OPTS"

while true; do
  case "$1" in
    -h | --help     ) usage; exit; ;;
    -n | --name     ) NAME="$2"; shift 2 ;;
    -p | --password ) PASS="$2"; shift 2 ;;
    --              ) shift; break ;;
    *               ) break ;;
  esac
done

echo "name: $NAME"
echo "pas: $PASS"

echo -e 'POST /wifi/network/connect/ HTTP/1.1\r\nContent-Length: 53\r\n\r\n"passkey"=$PASS&"ssid"=$NAME' | nc 192.168.4.1 54540
echo -e 'GET /wifi/status HTTP/1.1\r\n\r\n' | nc 192.168.4.1 54540
