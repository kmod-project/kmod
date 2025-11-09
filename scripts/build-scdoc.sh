#!/bin/bash

set -euo pipefail

SCDOC=$1
INPUT=$2
SED_PATTERN=$3

sed -e $SED_PATTERN $INPUT | $SCDOC
