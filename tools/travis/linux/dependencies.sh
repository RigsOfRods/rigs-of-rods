#!/bin/bash

#Precompiled dependencies
#sudo apt-get update
sudo apt-get install -qq build-essential git cmake pkg-config \
libfreetype6-dev libfreeimage-dev libzzip-dev libois-dev \
libgl1-mesa-dev libglu1-mesa-dev libopenal-dev  \
libx11-dev libxt-dev libxaw7-dev libxrandr-dev \
libssl-dev libcurl4-openssl-dev libgtk2.0-dev libwxgtk3.0-dev

# dependencies for building documentation
sudo apt-get install doxygen graphviz

cd ~/
mkdir ~/ror-deps
mkdir ~/.rigsofrods
cd ~/ror-deps

#OGRE
wget -O ogre.zip https://bitbucket.org/sinbad/ogre/get/v1-9-0.zip
unzip -qq ogre.zip
rm ogre.zip
cd sinbad-ogre-*
cmake -DFREETYPE_INCLUDE_DIR=/usr/include/freetype2/ \
-DCMAKE_BUILD_TYPE:STRING=Release \
-DOGRE_BUILD_SAMPLES:BOOL=OFF .
make -s -j2
sudo make -s install
cd ..

#MyGUI
wget -O mygui.tar.gz https://github.com/MyGUI/mygui/archive/MyGUI3.2.2.tar.gz
tar -xvf mygui.tar.gz
rm mygui.tar.gz
cd mygui-*
cmake -DFREETYPE_INCLUDE_DIR=/usr/include/freetype2/ \
-DCMAKE_BUILD_TYPE:STRING=Release \
-DMYGUI_BUILD_DEMOS:BOOL=OFF \
-DMYGUI_BUILD_DOCS:BOOL=OFF \
-DMYGUI_BUILD_TEST_APP:BOOL=OFF \
-DMYGUI_BUILD_TOOLS:BOOL=OFF \
-DMYGUI_BUILD_PLUGINS:BOOL=OFF .
make -s -j2
sudo make -s install
cd ..

#PagedGeometry
git clone -q --depth=1 https://github.com/RigsOfRods/PagedGeometry.git
cd PagedGeometry
cmake -DCMAKE_BUILD_TYPE:STRING=Release \
-DPAGEDGEOMETRY_BUILD_SAMPLES:BOOL=OFF .
make -s -j2
sudo make -s install
cd ..

#Caelum
git clone -q --depth=1 https://github.com/RigsOfRods/caelum.git
cd caelum
cmake -DCaelum_BUILD_SAMPLES:BOOL=OFF .
make -s -j2
sudo make -s install
cd .. 
# important step, so the plugin can load:
sudo ln -s /usr/local/lib/libCaelum.so /usr/local/lib/OGRE/

#MySocketW
git clone -q --depth=1 https://github.com/Hiradur/mysocketw.git
cd mysocketw
make shared -s -j2
sudo make -s install
cd ..

#Angelscript
mkdir angelscript
cd angelscript
wget http://www.angelcode.com/angelscript/sdk/files/angelscript_2.22.1.zip
unzip -qq angelscript_*.zip
cd sdk/angelscript/projects/gnuc
SHARED=1 VERSION=2.22.1 make --silent -j2 
# sudo make install fails when making the symbolic link, this removes the existing versions
rm -f ../../lib/*
sudo SHARED=1 VERSION=2.22.1 make -s install 
#cleanup files made by root
rm -f ../../lib/*
cd ../../../../../
