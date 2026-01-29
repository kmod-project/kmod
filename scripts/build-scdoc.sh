#!/bin/bash

set -euo pipefail

SCDOC=$1
shift
SED_PATTERN=$1
shift
INPUT=$*

sed -e $SED_PATTERN $INPUT | $SCDOC
