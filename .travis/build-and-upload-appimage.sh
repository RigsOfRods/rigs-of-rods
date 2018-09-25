#!/bin/sh

set -eu

# Only build AppImage if not building Pull Request
if [ "${TRAVIS_PULL_REQUEST}" = "false" ]
then
  # Get the appimagetool
  wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
  sudo chmod a+x ./appimagetool-x86_64.AppImage

  # Create the AppDir
  ninja install
  cp /usr/lib/x86_64-linux-gnu/libz.so  ./AppDir/usr/lib/
  cp /usr/lib/x86_64-linux-gnu/libzzip.so  ./AppDir/usr/lib/
  cp /usr/lib/x86_64-linux-gnu/libfreeimage.so  ./AppDir/usr/lib/
  ./appimagetool-x86_64.AppImage AppDir
  mv Rigs_of_Rods-x86_64.AppImage "RoR-V0.4.8.0-CIBuild-${TRAVIS_JOB_NUMBER}.AppImage"

  # Push AppImage to bintray
  curl -T "RoR-V0.4.8.0-CIBuild-${TRAVIS_JOB_NUMBER}.AppImage" -uanotherfoxguy:${API_KEY} https://api.bintray.com/content/anotherfoxguy/Rigs-of-Rods-CI-Build/CIBuild-Linux/0.4.8.0/RoR-V0.4.8.0-CIBuild-${TRAVIS_JOB_NUMBER}.AppImage?publish=1
fi
