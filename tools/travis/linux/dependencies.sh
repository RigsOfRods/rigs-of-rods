#!/bin/bash

#Precompiled dependencies
#sudo apt-get update
sudo apt-get install -qq subversion mercurial git automake cmake build-essential pkg-config doxygen \
 libfreetype6-dev libfreeimage-dev libzzip-dev scons libcurl4-openssl-dev \
 nvidia-cg-toolkit libgl1-mesa-dev libxrandr-dev libx11-dev libxt-dev libxaw7-dev \
 libglu1-mesa-dev libxxf86vm-dev uuid-dev libuuid1 libgtk2.0-dev \
 libopenal-dev libois-dev libssl-dev libwxgtk3.0-dev

cd ~/
mkdir ~/ror-deps
mkdir ~/.rigsofrods
cd ~/ror-deps

#OGRE
hg clone --quiet https://bitbucket.org/sinbad/ogre -b v1-8
cd ogre
cmake -DFREETYPE_INCLUDE_DIR=/usr/include/freetype2/ \
-DCMAKE_BUILD_TYPE:STRING=Release \
-DOGRE_BUILD_SAMPLES:BOOL=OFF .
make -s -j2
sudo make install
cd ..

#MyGUI
svn co -q https://svn.code.sf.net/p/my-gui/code/trunk my-gui -r 4344
cd my-gui
cmake -DFREETYPE_INCLUDE_DIR=/usr/include/freetype2/ \
-DCMAKE_BUILD_TYPE:STRING=Release \
-DMYGUI_BUILD_DEMOS:BOOL=OFF \
-DMYGUI_BUILD_DOCS:BOOL=OFF \
-DMYGUI_BUILD_TEST_APP:BOOL=OFF \
-DMYGUI_BUILD_TOOLS:BOOL=OFF \
-DMYGUI_BUILD_PLUGINS:BOOL=OFF .
make -s -j2
sudo make install
cd ..

#Paged Geometry
git clone -q --depth=1 https://github.com/Hiradur/ogre-paged.git
cd ogre-paged
cmake -DCMAKE_BUILD_TYPE:STRING=Release \
-DPAGEDGEOMETRY_BUILD_SAMPLES:BOOL=OFF .
make -s -j2
sudo make install
cd ..

#Caelum (needs specific revision for OGRE-1.8)
hg clone --quiet -r 3b0f1afccf5cb75c65d812d0361cce61b0e82e52 https://caelum.googlecode.com/hg/ caelum
cd caelum
cmake -DCaelum_BUILD_SAMPLES:BOOL=OFF .
make -s -j2
sudo make install
cd .. 
# important step, so the plugin can load:
sudo ln -s /usr/local/lib/libCaelum.so /usr/local/lib/OGRE/

#MySocketW
git clone -q --depth=1 https://github.com/Hiradur/mysocketw.git
cd mysocketw
make shared -s -j2
sudo make install
cd ..

#Angelscript
mkdir angelscript
cd angelscript
wget http://www.angelcode.com/angelscript/sdk/files/angelscript_2.22.1.zip
unzip angelscript_*.zip
cd sdk/angelscript/projects/gnuc
SHARED=1 VERSION=2.22.1 make --silent -j2 
# sudo make install fails when making the symbolic link, this removes the existing versions
rm -f ../../lib/*
sudo SHARED=1 VERSION=2.22.1 make install 
#cleanup files made by root
rm -f ../../lib/*
cd ../../../../../

#Hydrax (included in RoR's source tree)
#git clone --depth=1 https://github.com/imperative/CommunityHydrax.git
#cd CommunityHydrax
#make PREFIX=/usr/local
#sudo make install PREFIX=/usr/local
#cd ..
