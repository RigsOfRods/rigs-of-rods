#!/bin/sh

set -eu

if [ "${TRAVIS_PULL_REQUEST}" = "false" ]
then
	# Copy some libs
	cp /usr/lib/x86_64-linux-gnu/libzzip-0.so.13            ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libfreeimage.so.3          ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libjpegxr.so.0             ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libjxrglue.so.0            ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libraw.so.15               ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libwebp.so.5               ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libwebpmux.so.1            ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libIlmImf-2_2.so.22        ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libjasper.so.1             ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libHalf.so.12              ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libIex-2_2.so.12           ./redist/lib/
	cp /lib/x86_64-linux-gnu/libpng12.so.0                  ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libIlmThread-2_2.so.12     ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libCg.so                   ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libjpeg.so.8               ./redist/lib/
	cp /usr/lib/x86_64-linux-gnu/libXaw.so.7                ./redist/lib/

	# Get butler
	wget https://broth.itch.ovh/butler/linux-amd64/LATEST/archive/default -O butler.zip
	7z x butler.zip
	sudo chmod a+x ./butler

	# Setup redist folder and push it to itch
	ninja zip_and_copy_resources
	ninja install
	./butler push redist rigs-of-rods/rigs-of-rods-dev:linux-ci --userversion 0.4.8.0-CIBuild-${TRAVIS_JOB_NUMBER}
fi
