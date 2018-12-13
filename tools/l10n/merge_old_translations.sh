#!/bin/sh

# Merges old translations into new po template
# Setup:
# ./merge_old_translations.sh (this script)
# ./ror.pot (new template)
# ./languages/<lang_code>/LC_MESSAGES/ror.po (old translations)

# backup
cp -r languages languages.bak

# merge
for dir in languages/*/
do
	cd $dir
	cd LC_MESSAGES
	echo "Now merging $dir"
	msgmerge --update ror.po ../../../ror.pot
	cd ../../../
done
