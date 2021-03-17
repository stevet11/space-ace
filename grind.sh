#!/bin/bash

# valgrind -v --log-file=grind.log build/space-ace -l -u grinder
valgrind -v --leak-check=full --show-leak-kinds=all --log-file=grind.log build/space-ace -l -u grinder


tail grind.log
