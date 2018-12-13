#!/bin/sh
set -eu

# Moves po files from local tx repo to folder structure used by Rigs of Rods
# Requires:
# ./tx (Transifex folder initialized for Rigs of Rods project with up to date translations)
# ./languages/<lang_code>/LC_MESSAGES/ror.po (translations to be uploaded)

cd tx/translations/rigs-of-rods.rorpot
for file in *
do
	file="${file%.*}"
	echo "Now copying $file"
	mkdir -p ../../../languages/$file/LC_MESSAGES/
	cp $file.po ../../../languages/$file/LC_MESSAGES/ror.po
done
