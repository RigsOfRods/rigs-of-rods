#!/bin/bash

#Precompiled dependencies
#sudo apt-get update
sudo apt-get install -qq build-essential git cmake pkg-config \
libfreetype6-dev libfreeimage-dev libzzip-dev libois-dev \
libgl1-mesa-dev libglu1-mesa-dev libopenal-dev  \
libx11-dev libxt-dev libxaw7-dev libxrandr-dev \
libssl-dev libcurl4-openssl-dev libgtk2.0-dev libwxgtk3.0-dev
# libboost-all-dev (too old) nvidia-cg-toolkit (needed only at runtime, does not affect RoR build)

cd ~/
mkdir ~/ror-deps
mkdir ~/.rigsofrods
cd ~/ror-deps

#OGRE
wget -O ogre.zip http://bitbucket.org/sinbad/ogre/get/v1-8.zip
unzip ogre.zip
rm ogre.zip
cd sinbad-ogre-*
cmake -DFREETYPE_INCLUDE_DIR=/usr/include/freetype2/ \
-DCMAKE_BUILD_TYPE:STRING=Release \
-DOGRE_BUILD_SAMPLES:BOOL=OFF .
make -s -j2
sudo make -s install
cd ..

#MyGUI (needs specific revision)
wget -O mygui.zip https://github.com/MyGUI/mygui/archive/a790944c344c686805d074d7fc1d7fc13df98c37.zip
unzip mygui.zip
rm mygui.zip
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

#Paged Geometry
git clone -q --depth=1 https://github.com/Hiradur/ogre-paged.git
cd ogre-paged
cmake -DCMAKE_BUILD_TYPE:STRING=Release \
-DPAGEDGEOMETRY_BUILD_SAMPLES:BOOL=OFF .
make -s -j2
sudo make -s install
cd ..

#Caelum (needs specific revision for OGRE-1.8)
wget -O caelum.zip http://caelum.googlecode.com/archive/3b0f1afccf5cb75c65d812d0361cce61b0e82e52.zip
unzip caelum.zip
rm caelum.zip
cd caelum-*
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
unzip angelscript_*.zip
cd sdk/angelscript/projects/gnuc
SHARED=1 VERSION=2.22.1 make --silent -j2 
# sudo make install fails when making the symbolic link, this removes the existing versions
rm -f ../../lib/*
sudo SHARED=1 VERSION=2.22.1 make -s install 
#cleanup files made by root
rm -f ../../lib/*
cd ../../../../../

#Hydrax (included in RoR's source tree)
#git clone --depth=1 https://github.com/imperative/CommunityHydrax.git
#cd CommunityHydrax
#make PREFIX=/usr/local
#sudo make install PREFIX=/usr/local
#cd ..
