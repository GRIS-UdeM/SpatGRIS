#!/bin/sh

# Need Xcode 3.2.6 or later (pkgbuild and productbuild)
# with python 2.7.12 (32/64-bit) and wxpython 3.0.2.0 (classic) installed

# 1. update pyo sources
# 2. compile and install pyo float and double for python2
# 3. cd utils and build E-Pyo for python2
# 4. cd installers/osx and build the release for python2

export PACKAGE_NAME=SpatServerGris_v0.0.2.pkg
export DMG_DIR="SpatServerGris v0.0.2"
export DMG_NAME="SpatServerGris_v0.0.2.dmg"
export INSTALLER_DIR=`pwd`/installer
export APPLICATIONS_DIR=$INSTALLER_DIR/Application/Package_Contents/Applications
export SUPPORT_LIBS_DIR=$INSTALLER_DIR/SupportLibs/Package_Contents/usr/local
export BUILD_RESOURCES=$INSTALLER_DIR/PkgResources/English.lproj
export PKG_RESOURCES=$INSTALLER_DIR/../PkgResources

mkdir -p $APPLICATIONS_DIR
mkdir -p $SUPPORT_LIBS_DIR
mkdir -p $SUPPORT_LIBS_DIR/bin
mkdir -p $SUPPORT_LIBS_DIR/lib
mkdir -p $SUPPORT_LIBS_DIR/lib/jackmp
mkdir -p $SUPPORT_LIBS_DIR/lib/pkgconfig
mkdir -p $BUILD_RESOURCES

cp $PKG_RESOURCES/License.rtf $BUILD_RESOURCES/License.rtf
cp $PKG_RESOURCES/Welcome.rtf $BUILD_RESOURCES/Welcome.rtf
cp $PKG_RESOURCES/ReadMe.rtf $BUILD_RESOURCES/ReadMe.rtf

echo "copying application..."
cp -r ../Builds/MacOSX/build/Release/spatServerGris.app $APPLICATIONS_DIR/

echo "copying support libs..."
sudo cp /usr/local/bin/jackd $SUPPORT_LIBS_DIR/bin/
sudo cp /usr/local/bin/jackdmp $SUPPORT_LIBS_DIR/bin/
sudo cp /usr/local/bin/jack_metro $SUPPORT_LIBS_DIR/bin/
sudo cp /usr/local/bin/jack_lsp $SUPPORT_LIBS_DIR/bin/
sudo cp /usr/local/bin/jack_disconnect $SUPPORT_LIBS_DIR/bin/
sudo cp /usr/local/bin/jack_connect $SUPPORT_LIBS_DIR/bin/
sudo cp /usr/local/bin/jack_load $SUPPORT_LIBS_DIR/bin/
sudo cp /usr/local/bin/jack_unload $SUPPORT_LIBS_DIR/bin/
sudo cp /usr/local/bin/jack_netsource $SUPPORT_LIBS_DIR/bin/
sudo cp /usr/local/lib/jackmp/jack_coreaudio.so $SUPPORT_LIBS_DIR/lib/jackmp/
sudo cp /usr/local/lib/jackmp/jack_coremidi.so $SUPPORT_LIBS_DIR/lib/jackmp/
sudo cp /usr/local/lib/jackmp/jack_loopback.so $SUPPORT_LIBS_DIR/lib/jackmp/
sudo cp /usr/local/lib/jackmp/jack_net.so $SUPPORT_LIBS_DIR/lib/jackmp/
sudo cp /usr/local/lib/jackmp/jack_netone.so $SUPPORT_LIBS_DIR/lib/jackmp/
sudo cp /usr/local/lib/jackmp/netmanager.so $SUPPORT_LIBS_DIR/lib/jackmp/
sudo cp /usr/local/lib/jackmp/netadapter.so $SUPPORT_LIBS_DIR/lib/jackmp/
sudo cp /usr/local/lib/jackmp/audioadapter.so $SUPPORT_LIBS_DIR/lib/jackmp/
sudo cp /usr/local/lib/pkgconfig/jack.pc $SUPPORT_LIBS_DIR/lib/pkgconfig/
sudo cp /usr/local/lib/libjack.0.dylib $SUPPORT_LIBS_DIR/lib/
sudo cp /usr/local/lib/libjack.dylib $SUPPORT_LIBS_DIR/lib/
sudo cp /usr/local/lib/libjackserver.0.dylib $SUPPORT_LIBS_DIR/lib/
sudo cp /usr/local/lib/libjackserver.dylib $SUPPORT_LIBS_DIR/lib/
sudo cp /usr/local/lib/libjacknet.0.dylib $SUPPORT_LIBS_DIR/lib/
sudo cp /usr/local/lib/libjacknet.dylib $SUPPORT_LIBS_DIR/lib/

#sudo install_name_tool -change /usr/local/opt/flac/lib/libFLAC.8.dylib /usr/local/lib/libFLAC.8.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib
#sudo install_name_tool -change /usr/local/opt/libogg/lib/libogg.0.dylib /usr/local/lib/libogg.0.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib
#sudo install_name_tool -change /usr/local/opt/libvorbis/lib/libvorbis.0.dylib /usr/local/lib/libvorbis.0.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib
#sudo install_name_tool -change /usr/local/opt/libvorbis/lib/libvorbisenc.2.dylib /usr/local/lib/libvorbisenc.2.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib
#sudo install_name_tool -change /usr/local/opt/libogg/lib/libogg.0.dylib /usr/local/lib/libogg.0.dylib $SUPPORT_LIBS_DIR/libvorbis.0.dylib
#sudo install_name_tool -change /usr/local/opt/libogg/lib/libogg.0.dylib /usr/local/lib/libogg.0.dylib $SUPPORT_LIBS_DIR/libvorbisenc.2.dylib
#sudo install_name_tool -change /usr/local/Cellar/libvorbis/1.3.5/lib/libvorbis.0.dylib /usr/local/lib/libvorbis.0.dylib $SUPPORT_LIBS_DIR/libvorbisenc.2.dylib

cd $INSTALLER_DIR

echo "setting permissions..."
sudo chgrp -R wheel Application/Package_Contents/
sudo chown -R root Application/Package_Contents/
sudo chmod -R 755 Application/Package_Contents/

sudo chgrp -R wheel SupportLibs/Package_Contents/usr
sudo chown -R root SupportLibs/Package_Contents/usr
sudo chmod -R 755 SupportLibs/Package_Contents/usr

echo "building packages..."
pkgbuild    --identifier com.gris.umontreal.ca.spatServerGris.app.pkg \
            --root Application/Package_Contents/ \
            --version 1.0 \
            --scripts $PKG_RESOURCES \
            --component-plist ../Application.plist \
            Application.pkg

pkgbuild    --identifier com.gris.umontreal.ca.spatServerGris.libs.pkg \
            --root SupportLibs/Package_Contents/ \
            --version 1.0 \
            SupportLibs.pkg


echo "building product..."
productbuild --distribution ../Distribution.dist --resources $BUILD_RESOURCES $PACKAGE_NAME

echo "assembling DMG..."
mkdir "$DMG_DIR"
cd "$DMG_DIR"
cp ../$PACKAGE_NAME .
#cp -R ../../../../utils/E-Pyo_OSX_py2/E-Pyo.app .
#ln -s /Applications .
cd ..

hdiutil create "$DMG_NAME" -srcfolder "$DMG_DIR"

cd ..
mv installer/$DMG_NAME .

echo "clean up resources..."
#sudo rm -rf installer