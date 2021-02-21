#!/bin/bash

BASE=.$$images.h
FINAL=images.h

ansi-to-src ansi/*.ans > $BASE

# workaround
if [ -f "$FINAL" ]; then
  # file exists
  diff -q $BASE $FINAL
  status=$?
  if [ $status -eq 0 ]; then
    echo "no change..."
    # Clean up temp file
    rm -f $BASE    
  else
    mv -f $BASE $FINAL
  fi	  
else  
  # file doesn't exist, so no choice by to copy it over.	
  mv -f $BASE $FINAL
fi	

