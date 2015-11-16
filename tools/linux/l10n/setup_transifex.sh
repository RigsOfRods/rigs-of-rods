#!/bin/sh
set -eu

# Set's up local repo for Transifex

mkdir tx
cd tx

tx init --host=www.transifex.com
tx set --auto-remote https://www.transifex.com/projects/p/rigs-of-rods
tx pull -a
