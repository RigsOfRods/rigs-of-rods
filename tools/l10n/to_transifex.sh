#!/bin/sh
set -eu

# Takes po files from languages folder and uploads them to Transifex
# Requires:
# ./tx (Transifex folder initialized for Rigs of Rods project)
# ./languages/<lang_code>/LC_MESSAGES/ror.po (translations to be uploaded)

cd languages
for dir in *
do
	echo "Now copying $dir"
	cp $dir/LC_MESSAGES/ror.po ../tx/translations/rigs-of-rods.rorpot/$dir.po
done

# danger zone, check everything went well first
# and make a backup (Transifex has no version control)
cd ../tx
# tx push -t --skip

# Don't forget to update source file (ror.pot) if necessary
