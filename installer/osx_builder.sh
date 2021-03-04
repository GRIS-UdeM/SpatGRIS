#!/bin/sh

# Developer: Olivier Belanger & Samuel Béland

# To build the installer for OSX, compile SpatGRIS in Release mode
# and run this shell script from the installer folder while giving
# the version number as an argument.



if [ "$#" -ne 1 ]; then
    echo "Please provide the SpatGRIS version number as an argument to this script"
    exit -1
fi

VERSION=$1
IDENTIFIER="ca.umontreal.musique.gris.spatgris"

export PACKAGE_NAME="SpatGRIS_v$VERSION.pkg"
export DMG_DIR="SpatGRIS_v$VERSION"
export DMG_NAME="SpatGRIS_v$VERSION.dmg"

export INSTALLER_DIR=`pwd`/installerdir
export APPLICATIONS_DIR=$INSTALLER_DIR/Application/Package_Contents/Applications
# export PLUGINS_DIR=$INSTALLER_DIR/Plugins/Package_Contents/Library/Audio/Plug-Ins

export BUILD_RESOURCES=$INSTALLER_DIR/PkgResources/English.lproj
export PKG_RESOURCES=$INSTALLER_DIR/../PkgResources

mkdir -p $APPLICATIONS_DIR
# mkdir -p $PLUGINS_DIR
# mkdir -p $PLUGINS_DIR/HAL
# mkdir -p $PLUGINS_DIR/VST
# mkdir -p $PLUGINS_DIR/Components
mkdir -p $BUILD_RESOURCES

cp $PKG_RESOURCES/License.rtf $BUILD_RESOURCES/License.rtf
cp $PKG_RESOURCES/Welcome.rtf $BUILD_RESOURCES/Welcome.rtf
cp $PKG_RESOURCES/ReadMe.rtf $BUILD_RESOURCES/ReadMe.rtf

echo "copying application..."
cp -r ../Builds/MacOSX/build/Release/SpatGRIS.app $APPLICATIONS_DIR/

cd $INSTALLER_DIR

echo "building packages..."
pkgbuild    --identifier $IDENTIFIER.pkg \
            --root Application/Package_Contents/ \
            --version $VERSION \
            --scripts $PKG_RESOURCES \
            --component-plist ../Application.plist \
            --sign "Q2A837SX87" \
            Application.pkg

# pkgbuild    --identifier com.gris.umontreal.ca.SpatGRIS.plugins.pkg \
#             --root Plugins/Package_Contents/ \
#             --version $VERSION \
#             Plugins.pkg

echo "building product..."
productbuild --distribution ../Distribution.dist --resources $BUILD_RESOURCES $PACKAGE_NAME

echo "assembling DMG..."
mkdir "$DMG_DIR"
cd "$DMG_DIR"
cp ../$PACKAGE_NAME .
cp -r "../../../Resources/templates" .

cd ..

hdiutil create "$DMG_NAME" -srcfolder "$DMG_DIR"

cd ..
mv $INSTALLER_DIR/$DMG_NAME .

echo "clean up resources..."
sudo rm -rf $INSTALLER_DIR
