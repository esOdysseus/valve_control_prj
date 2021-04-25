#!/bin/bash

echo "Pre-Date"
date

sudo date +%T -s "${1}"

echo "Post-Date"
date

