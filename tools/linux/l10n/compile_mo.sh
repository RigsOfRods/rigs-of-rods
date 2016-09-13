#!/bin/sh

# Compiles translated *.po files to binary *.mo files for gettext
# Setup:
# ./compile_mo.sh (this script)
# ./languages/<lang_code>/LC_MESSAGES/ror.po (po file to be baked into *.mo file)

cd languages
for dir in *
do
	cd $dir
	cd LC_MESSAGES
	echo "Now compiling $dir"
	msgfmt -o ror.mo ror.po
	rm ror.po
	cd ../../
done
