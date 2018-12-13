#!/bin/sh
set -eu

# Merges translations from local with Transifex translations
# Setup:
# ./merge_from_tx.sh (this script)
# ./languages/<lang_code>/LC_MESSAGES/ror.po (old translations)
# ./tx/translations/rigs-of-rods.rorpot/*.po (new translations)

# merge
cd languages
for dir in *
do
	cd $dir/LC_MESSAGES
	echo "Now merging $dir"
	msgmerge --update ../../../tx/translations/rigs-of-rods.rorpot/$dir.po ror.po
	cd ../../
done
