#!/bin/sh
set -eu

# extracts all strings inside _L() macro to a .pot file as a template for translations

find ../../../source -iname "*.cpp" | xargs xgettext -o ror.pot -k_L -s