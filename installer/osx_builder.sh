#!/bin/sh

# Developers: Olivier Belanger

# To build the installer for OSX, install JackOSX.0.92_b3.pkg,
# compile ServerGris in Release mode
# and run this shell script from the installer folder.

export PACKAGE_NAME=ServerGris_v2.0.2.pkg
export DMG_DIR="ServerGris v2.0.2"
export DMG_NAME="ServerGris_v2.0.2.dmg"

export INSTALLER_DIR=`pwd`/installerdir
export APPLICATIONS_DIR=$INSTALLER_DIR/Application/Package_Contents/Applications
export SUPPORT_LIBS_DIR=$INSTALLER_DIR/SupportLibs/Package_Contents/usr/local
export FRAMEWORKS_DIR=$INSTALLER_DIR/Frameworks/Package_Contents/Library/Frameworks
export PLUGINS_DIR=$INSTALLER_DIR/Plugins/Package_Contents/Library/Audio/Plug-Ins

export BUILD_RESOURCES=$INSTALLER_DIR/PkgResources/English.lproj
export PKG_RESOURCES=$INSTALLER_DIR/../PkgResources

mkdir -p $APPLICATIONS_DIR
mkdir -p $SUPPORT_LIBS_DIR
mkdir -p $FRAMEWORKS_DIR
mkdir -p $PLUGINS_DIR
mkdir -p $SUPPORT_LIBS_DIR/bin
mkdir -p $SUPPORT_LIBS_DIR/lib
mkdir -p $SUPPORT_LIBS_DIR/include
mkdir -p $SUPPORT_LIBS_DIR/lib/jackmp
mkdir -p $SUPPORT_LIBS_DIR/lib/pkgconfig
mkdir -p $PLUGINS_DIR/HAL
mkdir -p $PLUGINS_DIR/VST
mkdir -p $PLUGINS_DIR/Components
mkdir -p $BUILD_RESOURCES

cp $PKG_RESOURCES/License.rtf $BUILD_RESOURCES/License.rtf
cp $PKG_RESOURCES/Welcome.rtf $BUILD_RESOURCES/Welcome.rtf
cp $PKG_RESOURCES/ReadMe.rtf $BUILD_RESOURCES/ReadMe.rtf

echo "copying application..."
cp -r ../Builds/MacOSX/build/Release/ServerGris.app $APPLICATIONS_DIR/
cp -r /Applications/Jack $APPLICATIONS_DIR/

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
sudo cp -r /usr/local/include/jack $SUPPORT_LIBS_DIR/include/

sudo cp -r /Library/Frameworks/Jackmp.framework $FRAMEWORKS_DIR
sudo cp -r /Library/Frameworks/Jackservermp.framework $FRAMEWORKS_DIR
sudo cp -r /Library/Frameworks/Jacknet.framework $FRAMEWORKS_DIR
sudo cp -r /Library/Frameworks/Panda.framework $FRAMEWORKS_DIR

sudo cp /usr/local/lib/libjack.0.dylib $SUPPORT_LIBS_DIR/lib/
sudo cp /usr/local/lib/libjack.dylib $SUPPORT_LIBS_DIR/lib/
sudo cp /usr/local/lib/libjackserver.0.dylib $SUPPORT_LIBS_DIR/lib/
sudo cp /usr/local/lib/libjackserver.dylib $SUPPORT_LIBS_DIR/lib/
sudo cp /usr/local/lib/libjacknet.0.dylib $SUPPORT_LIBS_DIR/lib/
sudo cp /usr/local/lib/libjacknet.dylib $SUPPORT_LIBS_DIR/lib/

sudo cp -r /Library/Audio/Plug-Ins/HAL/JackRouter.plugin $PLUGINS_DIR/HAL/
sudo cp -r /Library/Audio/Plug-Ins/Components/JACK-insert.component $PLUGINS_DIR/Components/
sudo cp -r /Library/Audio/Plug-Ins/VST/JACK-insert.vst $PLUGINS_DIR/VST/

cd $INSTALLER_DIR

echo "setting permissions..."
#sudo chgrp -R wheel Application/Package_Contents/
#sudo chown -R root Application/Package_Contents/
#sudo chmod -R 755 Application/Package_Contents/
sudo chgrp -R wheel SupportLibs/Package_Contents/usr
sudo chown -R root SupportLibs/Package_Contents/usr
sudo chmod -R 755 SupportLibs/Package_Contents/usr

echo "building packages..."
pkgbuild    --identifier com.gris.umontreal.ca.ServerGris.app.pkg \
            --root Application/Package_Contents/ \
            --version 1.0 \
            --scripts $PKG_RESOURCES \
            --component-plist ../Application.plist \
            Application.pkg

pkgbuild    --identifier com.gris.umontreal.ca.ServerGris.libs.pkg \
            --root SupportLibs/Package_Contents/ \
            --version 1.0 \
            SupportLibs.pkg

pkgbuild    --identifier com.gris.umontreal.ca.ServerGris.frameworks.pkg \
            --root Frameworks/Package_Contents/ \
            --version 1.0 \
            Frameworks.pkg

pkgbuild    --identifier com.gris.umontreal.ca.ServerGris.plugins.pkg \
            --root Plugins/Package_Contents/ \
            --version 1.0 \
            Plugins.pkg

echo "building product..."
productbuild --distribution ../Distribution.dist --resources $BUILD_RESOURCES $PACKAGE_NAME

echo "assembling DMG..."
mkdir "$DMG_DIR"
cd "$DMG_DIR"
cp ../$PACKAGE_NAME .
cp -r "../../../Resources/ServerGRIS Templates" .

cd ..

hdiutil create "$DMG_NAME" -srcfolder "$DMG_DIR"

cd ..
mv $INSTALLER_DIR/$DMG_NAME .

echo "clean up resources..."
sudo rm -rf $INSTALLER_DIR
