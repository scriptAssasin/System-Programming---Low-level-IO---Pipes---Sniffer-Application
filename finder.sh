#!/bin/bash

echo "The total number of strings contain: $1 is:"
grep -R $1 ./outputs | cut -d' ' -f2 | awk '{ SUM += $1} END { print SUM }'
