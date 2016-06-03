#!/bin/sh

set -eu

mkdir $DEPS_BUILD_DIR
mkdir $DEPS_INSTALL_DIR

#OGRE
cd $DEPS_BUILD_DIR
wget -O ogre.zip https://bitbucket.org/sinbad/ogre/get/v1-9.zip
unzip -qq ogre.zip && rm ogre.zip && mv sinbad-ogre-* ogre
cd ogre
cmake -DCMAKE_INSTALL_PREFIX=$DEPS_INSTALL_DIR \
-DCMAKE_BUILD_TYPE:STRING=Release \
-DCMAKE_CXX_FLAGS="-w -O0 -pipe" \
-DOGRE_BUILD_TOOLS=OFF \
-DOGRE_BUILD_SAMPLES:BOOL=OFF .
time make -s -j2
make install

#MyGUI
cd $DEPS_BUILD_DIR
wget -O mygui.tar.gz https://github.com/MyGUI/mygui/archive/MyGUI3.2.2.tar.gz
tar -xvf mygui.tar.gz && rm mygui.tar.gz && mv mygui-* mygui
cd mygui
cmake -DCMAKE_INSTALL_PREFIX=$DEPS_INSTALL_DIR \
-DCMAKE_PREFIX_PATH=$DEPS_INSTALL_DIR \
-DCMAKE_BUILD_TYPE:STRING=Release \
-DCMAKE_CXX_FLAGS="-w -O0 -pipe" \
-DMYGUI_BUILD_DEMOS:BOOL=OFF \
-DMYGUI_BUILD_DOCS:BOOL=OFF \
-DMYGUI_BUILD_TEST_APP:BOOL=OFF \
-DMYGUI_BUILD_TOOLS:BOOL=OFF \
-DMYGUI_BUILD_PLUGINS:BOOL=OFF .
time make -s -j2
make install

#PagedGeometry
cd $DEPS_BUILD_DIR
git clone -q --depth=1 https://github.com/RigsOfRods/ogre-pagedgeometry.git
cd ogre-pagedgeometry
cmake -DCMAKE_INSTALL_PREFIX=$DEPS_INSTALL_DIR \
-DCMAKE_PREFIX_PATH=$DEPS_INSTALL_DIR \
-DCMAKE_BUILD_TYPE:STRING=Release \
-DCMAKE_CXX_FLAGS="-w -O0 -pipe" \
-DPAGEDGEOMETRY_BUILD_SAMPLES:BOOL=OFF .
time make -s -j2
make install

#Caelum
cd $DEPS_BUILD_DIR
git clone -q --depth=1 https://github.com/RigsOfRods/caelum.git
cd caelum
cmake -DCMAKE_INSTALL_PREFIX=$DEPS_INSTALL_DIR \
-DCMAKE_PREFIX_PATH=$DEPS_INSTALL_DIR \
-DCMAKE_BUILD_TYPE:STRING=Release \
-DCMAKE_CXX_FLAGS="-w -O0 -pipe" \
-DCaelum_BUILD_SAMPLES:BOOL=OFF .
time make -s -j2
make install
# important step, so the plugin can load:
#sudo ln -s /usr/local/lib/libCaelum.so /usr/local/lib/OGRE/

#MySocketW
cd $DEPS_BUILD_DIR
git clone -q --depth=1 https://github.com/Hiradur/mysocketw.git
mkdir -p mysocketw/build
cd mysocketw/build
cmake -DCMAKE_INSTALL_PREFIX=$DEPS_INSTALL_DIR \
-DCMAKE_BUILD_TYPE:STRING=Release \
-DCMAKE_CXX_FLAGS="-w -O0 -pipe" \
..
time make -s -j2
make install

#Angelscript
cd $DEPS_BUILD_DIR
mkdir angelscript
cd angelscript
wget http://www.angelcode.com/angelscript/sdk/files/angelscript_2.22.1.zip
unzip -qq angelscript_*.zip
cd sdk/angelscript/projects/cmake
cmake -DCMAKE_INSTALL_PREFIX=$DEPS_INSTALL_DIR \
-DCMAKE_BUILD_TYPE:STRING=Release \
-DCMAKE_CXX_FLAGS="-w -O0 -pipe" \
.
time make -s -j2 
cp -r ../../lib/libAngelscript.a $DEPS_INSTALL_DIR/lib/libangelscript.a
cp -r ../../include $DEPS_INSTALL_DIR

ls $DEPS_INSTALL_DIR/lib
# sudo make install fails when making the symbolic link, this removes the existing versions
#rm -f ../../lib/*
#make -s install 
#cleanup files made by root
#rm -f ../../lib/*
