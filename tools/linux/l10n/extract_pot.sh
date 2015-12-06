#!/bin/sh
set -eu

# extract strings preceded by a specific macro to a .pot file as a template for translations

# Configurator uses wxLocale and as such is bound to _() macro
find ../../../source/configurator -iname "*.cpp" | xargs xgettext -o ror.pot -k_ -s

# RoR uses the custom _L() macro 
find ../../../source -iname "*.cpp" | xargs xgettext -o ror.pot -k_L -s -j
