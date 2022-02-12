#!/bin/sh

# Developer: Olivier Belanger & Samuel Béland

#==============================================================================
export USAGE="usage:\n\tosx_builder --path <bin-path> --plugins <pugins-pkg-path> --blackhole <blackhole-pkg-path> --pass <dev-id-password>"
export BIN_PATH=""
export PASS=""
export PLUGINS_PKG=""
export BLACKHOLE_PKG=""

#==============================================================================
# Parse args

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    --path)
    BIN_PATH="$2"
    shift # past argument
    shift # past value
    ;;
    --pass)
    PASS="$2"
    shift # past argument
    shift # past value
    ;;
    --plugins)
	PLUGINS_PKG="$2"
	shift
	shift
	;;
	--blackhole)
	BLACKHOLE_PKG="$2"
	shift
	shift
	;;
    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

if [[ "$BIN_PATH" == "" ]];then
	echo "Missing param --path"
	echo -e "$USAGE"
	exit 1
elif [[ "$PASS" == "" ]];then
	echo "Missing param --pass"
	echo -e "$USAGE"
	exit 1
elif [[ $PLUGINS_PKG == "" ]];then
	echo "Missing param --plugins"
	echo -e "$USAGE"
	exit 1
elif [[ $BLACKHOLE_PKG == "" ]];then
	echo "Missing param --backhole"
	echo -e "$USAGE"
	exit 1
fi

#==============================================================================
# get app version

PROJECT_FILE="../SpatGRIS.jucer"
VERSION=`Projucer --get-version "$PROJECT_FILE"`
echo "Version is $VERSION"

#==============================================================================
# package app

IDENTIFIER="ca.umontreal.musique.gris.spatgris.pkg"
installerSignature="Developer ID Installer: Gael Lane Lepine (62PMMWH49Z)"
appSignature="Developer ID Application: Gael Lane Lepine (62PMMWH49Z)"

export notarizeUser="glanelepine@gmail.com"
export identifier="ca.umontreal.musique.gris.spatgris.installer"
export PACKAGE_NAME="SpatGRIS_v$VERSION.pkg"
export DMG_DIR="SpatGRIS_v$VERSION"
export DMG_NAME="SpatGRIS_v$VERSION.dmg"

export INSTALLER_DIR=`pwd`/installerdir
export APPLICATIONS_DIR=$INSTALLER_DIR/Application/Package_Contents/Applications
export PLUGINS_DIR=$INSTALLER_DIR/Plugins/Package_Contents/Library/Audio/Plug-Ins

export BUILD_RESOURCES=$INSTALLER_DIR/PkgResources/English.lproj
export PKG_RESOURCES=$INSTALLER_DIR/../PkgResources

function build_package() {
	mkdir -p $APPLICATIONS_DIR || exit 1
	mkdir -p $BUILD_RESOURCES || exit 1

	cp $PKG_RESOURCES/License.rtf $BUILD_RESOURCES/License.rtf || exit 1
	cp $PKG_RESOURCES/Welcome.rtf $BUILD_RESOURCES/Welcome.rtf || exit 1
	cp $PKG_RESOURCES/ReadMe.rtf $BUILD_RESOURCES/ReadMe.rtf || exit 1

	echo "copying application..."
	cp -r $BIN_PATH $APPLICATIONS_DIR/ || exit 1

	echo "Copying plugins..."
	cp -r $PLUGINS_PKG "$INSTALLER_DIR/Plugins.pkg"

	cd $INSTALLER_DIR

	echo "building Application.pkg"
	pkgbuild    --identifier "$IDENTIFIER" \
	            --root "Application/Package_Contents/" \
	            --version "$VERSION" \
	            --scripts "$PKG_RESOURCES" \
	            --component-plist "../Application.plist" \
	            --sign "$installerSignature" \
	            --timestamp \
	            "Application.pkg" || exit 1

	echo "building $PACKAGE_NAME"
	productbuild 	--distribution "../Distribution.xml" \
					--resources "$BUILD_RESOURCES" \
					--sign "$installerSignature" \
					--timestamp \
					--version "$VERSION" \
					"$PACKAGE_NAME" || exit 1
}

#==============================================================================
function build_dmg() {
	echo "assembling DMG..."
	mkdir -p "$DMG_DIR" || exit 1
	cd "$DMG_DIR" || exit 1
	cp "../$PACKAGE_NAME" . || exit 1
	cp "$BLACKHOLE_PKG" . || exit -1

	cd ..

	hdiutil create "$DMG_NAME" -srcfolder "$DMG_DIR" || exit 1

	cd ..
	mv $INSTALLER_DIR/$DMG_NAME . || exit 1

	# sign dmg
	echo "Signing dmg"
	codesign -s "$appSignature" "$DMG_NAME" --timestamp --verbose || exit 1
}

#==============================================================================
function send_for_notarisation() {
	#ZIP_FILE="$PACKAGE_NAME.zip"

	#zip -r "$ZIP_FILE" "$PACKAGE_NAME"

	echo "Sending to notarization authority..."
	xcrun altool --notarize-app --primary-bundle-id "$identifier" -u "$notarizeUser" -p "$PASS" --file "$DMG_NAME"
	#rm "$ZIP_FILE"
}

#==============================================================================
function get_last_request_uuid() {
	history=`xcrun altool --notarization-history 0 -u "$notarizeUser" -p "$PASS"`
	echo "$history" | head -n 6 | tail -n 1 | cut -d' ' -f 4
}

#==============================================================================
function wait_for_notarization() {
	echo "Checking for notarization success..."
	echo "waiting a bit..."
	sleep 30
	WAITING=" in progress"
	SUCCESS=" success"
	uuid=`get_last_request_uuid`
	status="$WAITING"
	while [[ "$status" == "$WAITING" ]];do
		sleep 10
		history=`xcrun altool --notarization-info "$uuid" -u "$notarizeUser" -p "$PASS"`
		status=`echo "$history" | grep Status | head -n 1 | cut -d: -f 2`
		echo "Status is \"$status\""
	done
	if [[ "$status" != "$SUCCESS" ]];then
		echo -e "Error : notarization was refused, see the report:\n"
		xcrun altool \
			--notarization-info "$uuid" \
			-u "$notarizeUser" \
			-p "$PASS"
		exit 1
	fi
}

#==============================================================================
function staple() {
	echo "Rubber stamping spatGRIS..."
	xcrun stapler staple "$DMG_NAME" || exit 1
}

#==============================================================================
function cleanup() {
	echo "cleaning up resources..."
	rm -rf $INSTALLER_DIR
	rm -fr Plugins.pkg
}

#==============================================================================
build_package
build_dmg
send_for_notarisation
wait_for_notarization
staple
# cleanup