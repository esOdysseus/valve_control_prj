#!/bin/bash

_RED_='\033[0;31m'
_GREEN_='\033[0;32m'
_BLUE_='\033[0;34m'
_YELLOW_='\033[0;33m'
_NO_COLOR_='\033[0m'


function loge { printf "[E] ${_RED_}$1${_NO_COLOR_}\n"; exit 0; }
function logw { printf "[W] ${_YELLOW_}$1${_NO_COLOR_}\n"; }
function logi { printf "[I] ${_GREEN_}$1${_NO_COLOR_}\n"; }
function logd { printf "[D] ${_BLUE_}$1${_NO_COLOR_}\n"; }


function get_chr() {
    if [ "${1}" -ge "256" ] ; then
        echo "NULL"
        exit 0
    fi
    printf  "\\$(printf '%03o' "${1}")"
}
function get_asc() {
    LC_CTYPE=C printf '%d' "'${1}'"
}

