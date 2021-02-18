#!/bin/bash

valgrind -v --log-file=grind.log build/space-ace -l -u grinder


tail grind.log
