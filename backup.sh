#!/bin/bash

# Recommended RAR parameters

# RAR4="-m5 -dh -ed -mct -ow -t -s -rr10p"
RAR4="-m5 -dh -mct -ow -t -s -rr10p"

# RAR5="-ma5 -qo+ -md128M -m5 -dh -ed -ow -t -s -rr10p -idp"
# RAR5="-ma5 -qo+ -md128M -m5 -dh -ow -t -s -rr10p -idp"

rar a $RAR4 -r -ola -x*.rar -x*.log -xbuild/* -xbackup/* -xdbuild/* -agYYYY-MM-DD-NN door++ 

