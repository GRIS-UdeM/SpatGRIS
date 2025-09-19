#!/bin/sh

# Developers: Gaël Lane Lépine, Olivier Belanger, Samuel Béland

#==============================================================================
export USAGE="usage:\n\tosx_builder --path <bin-path> --plugins <plugins-pkg-path> --blackhole <blackhole-pkgs-dir-path> --speakerview <speakerview-app-path> --pass <dev-id-password>"
export BIN_PATH=""
export PASS=""
export PLUGINS_PKG=""
export BLACKHOLE_PKGS_DIR=""
export SPEAKERVIEW_APP=""
export MOVE_SG_TO_FOREGROUND_DIR=""

Projucer=~/JUCE/Projucer.app/Contents/MacOS/Projucer

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
	BLACKHOLE_PKGS_DIR="$2""/*"
	shift
	shift
	;;
	--speakerview)
	SPEAKERVIEW_APP="$2"
	shift
	shift
	;;
	--movetoforeground)
	MOVE_SG_TO_FOREGROUND_DIR="$2"
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
	echo "$USAGE"
	exit 1
elif [[ "$PASS" == "" ]];then
	echo "Missing param --pass"
	echo "$USAGE"
	exit 1
elif [[ $PLUGINS_PKG == "" ]];then
	echo "Missing param --plugins"
	echo "$USAGE"
	exit 1
elif [[ $BLACKHOLE_PKGS_DIR == "" ]];then
	echo "Missing param --backhole"
	echo "$USAGE"
	exit 1
elif [[ $SPEAKERVIEW_APP == "" ]];then
	echo "Missing param --speakerview"
	echo "$USAGE"
	exit 1
elif [[ $MOVE_SG_TO_FOREGROUND_DIR == "" ]];then
	echo "Missing param --movetoforeground"
	echo "$USAGE"
	exit 1
fi

#==============================================================================
# get app version

PROJECT_FILE="../SpatGRIS.jucer"
VERSION=`$Projucer --get-version "$PROJECT_FILE"`
echo "SpatGris version is $VERSION"

#==============================================================================
# get plugins version

PLUGINS_VERSION=$(echo $PLUGINS_PKG | awk -F 'ControlGRIS_' '{print $NF}' | awk -F '.pkg' '{print $1}')
echo "ControlGris version is $PLUGINS_VERSION"

#==============================================================================
# get SpeakerView version

SPEAKERVIEW_VERSION=`plutil -p $SPEAKERVIEW_APP/Contents/Info.plist | grep CFBundleShortVersionString | grep -o '"[[:digit:].]*"' | sed 's/"//g'`
echo "SpeakerView Forward version is $SPEAKERVIEW_VERSION"

#==============================================================================
# get moveSGToForeground version

MSGTF_VERSION=`$MOVE_SG_TO_FOREGROUND_DIR/moveSGToForegroundMacOS.sh -v`
echo "moveSGToForeground version is $MSGTF_VERSION"

#==============================================================================
# package app

IDENTIFIER="ca.umontreal.musique.gris.spatgris.pkg"
SPEAKERVIEW_IDENTIFIER="ca.umontreal.musique.gris.speakerview.pkg"
MSGTF_IDENTIFIER="ca.umontreal.musique.gris.movesgtoforeground.pkg"
installerSignature="Developer ID Installer: Gael Lane Lepine (62PMMWH49Z)"
appSignature="Developer ID Application: Gael Lane Lepine (62PMMWH49Z)"

export notarizeUser="glanelepine@gmail.com"
export identifier="ca.umontreal.musique.gris.spatgris.installer"
export teamId="62PMMWH49Z"
export PACKAGE_NAME="SpatGRIS_v$VERSION.pkg"
export DMG_DIR="SpatGRIS_v$VERSION"
export DMG_NAME="SpatGRIS_v$VERSION.dmg"

export INSTALLER_DIR=`pwd`/installerdir
export APPLICATIONS_DIR=$INSTALLER_DIR/Application/Package_Contents/Applications/GRIS
export SPEAKERVIEW_APPLICATIONS_DIR=$INSTALLER_DIR/SpeakerView_Application/Package_Contents/Applications/GRIS
export MSGTF_APPLICATIONS_DIR=$INSTALLER_DIR/MSGTF_Application/Package_Contents/Applications/GRIS/utilities/MSGTF
export PLUGINS_DIR=$INSTALLER_DIR/Plugins/Package_Contents/Library/Audio/Plug-Ins

export BUILD_RESOURCES=$INSTALLER_DIR/PkgResources/English.lproj
export PKG_RESOURCES=$INSTALLER_DIR/../PkgResources

export SV_PKG_RESOURCES=$INSTALLER_DIR/../SV_PkgResources

function build_package() {
	mkdir -p $APPLICATIONS_DIR || exit 1
	mkdir -p $SPEAKERVIEW_APPLICATIONS_DIR || exit 1
	mkdir -p $MSGTF_APPLICATIONS_DIR || exit 1
	mkdir -p $BUILD_RESOURCES || exit 1

	cp $PKG_RESOURCES/License.rtf $BUILD_RESOURCES/License.rtf || exit 1
	cp $PKG_RESOURCES/Welcome.rtf $BUILD_RESOURCES/Welcome.rtf || exit 1
	cp $PKG_RESOURCES/ReadMe.rtf $BUILD_RESOURCES/ReadMe.rtf || exit 1

	echo "copying application..."
	cp -r $BIN_PATH $APPLICATIONS_DIR/ || exit 1

	echo "copying SpeakerView Forward application..."
	cp -r $SPEAKERVIEW_APP $SPEAKERVIEW_APPLICATIONS_DIR/ || exit 1

	echo "copying and signing moveSGTOForeground script..."
	cp -r $MOVE_SG_TO_FOREGROUND_DIR $MSGTF_APPLICATIONS_DIR/ || exit 1
	codesign -f -s "$appSignature" "$MSGTF_APPLICATIONS_DIR/moveSGToForegroundMacOS.sh" \
		--timestamp --options=runtime --verbose

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

	echo "building SpeakerView.pkg"
	pkgbuild    --identifier "$SPEAKERVIEW_IDENTIFIER" \
	            --root "SpeakerView_Application/Package_Contents/" \
	            --version "$SPEAKERVIEW_VERSION" \
				--scripts "$SV_PKG_RESOURCES" \
	            --component-plist "../SpeakerView.plist" \
	            --sign "$installerSignature" \
	            --timestamp \
	            "SpeakerView.pkg" || exit 1

	echo "building moveSGToForeground.pkg"
	pkgbuild	--identifier "$MSGTF_IDENTIFIER" \
				--root "MSGTF_Application/Package_Contents/" \
				--version "$MSGTF_VERSION" \
				--sign "$installerSignature" \
				--timestamp \
				"MoveSGToForeground.pkg" || exit 1

	echo "adding SpatGris, SpeakerView, ControlGris versions to installer"
	sed -i '' "s/title=\"SpatGRIS.*\"/title=\"SpatGRIS $VERSION\"/" ../Distribution.xml || exit 1
	sed -i '' "s/title=\"SpeakerView.*\"/title=\"SpeakerView $SPEAKERVIEW_VERSION\"/" ../Distribution.xml || exit 1
	sed -i '' "s/title=\"ControlGRIS.*\"/title=\"ControlGRIS $PLUGINS_VERSION\"/" ../Distribution.xml || exit 1

	echo "building $PACKAGE_NAME"
	productbuild 	--distribution "../Distribution.xml" \
					--resources "$BUILD_RESOURCES" \
					--sign "$installerSignature" \
					--timestamp \
					--version "$VERSION" \
					"$PACKAGE_NAME" || exit 1

	echo "resetting Distribution.xml file"
	sed -i '' "s/title=\"SpatGRIS.*\"/title=\"SpatGRIS\"/" ../Distribution.xml || exit 1
	sed -i '' "s/title=\"SpeakerView.*\"/title=\"SpeakerView\"/" ../Distribution.xml || exit 1
	sed -i '' "s/title=\"ControlGRIS.*\"/title=\"ControlGRIS\"/" ../Distribution.xml || exit 1
}

#==============================================================================


#==============================================================================
function build_dmg() {
	echo "assembling DMG..."
	mkdir -p "$DMG_DIR" || exit 1
	cd "$DMG_DIR" || exit 1
	cp "../$PACKAGE_NAME" . || exit 1

	for blackhole_pkg in $BLACKHOLE_PKGS_DIR; do
		cp $blackhole_pkg . || exit -1
	done

	cd ..

	hdiutil create -size 400m "$DMG_NAME" -srcfolder "$DMG_DIR" || exit 1

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
	xcrun notarytool submit --apple-id "$notarizeUser" --password "$PASS" --team-id $teamId "$DMG_NAME"
	#rm "$ZIP_FILE"
}

#==============================================================================
function get_last_request_uuid() {
	checkHistory=`xcrun notarytool history --apple-id "$notarizeUser" --password "$PASS" --team-id $teamId`
	echo "$checkHistory" | head -n 5 | tail -n 1 | cut -d' ' -f 6
}

#==============================================================================
function wait_for_notarization() {
	echo "Checking for notarization success..."
	echo "waiting a bit..."
	sleep 10
	WAITING=" In Progress"
	SUCCESS=" Accepted"
	uuid=`get_last_request_uuid`
	checkStatus="$WAITING"
	while [[ "$checkStatus" == "$WAITING" ]];do
		sleep 10
		checkHistory=`xcrun notarytool info "$uuid" --apple-id "$notarizeUser" --password "$PASS" --team-id $teamId`
		checkStatus=`echo "$checkHistory" | grep status | head -n 1 | cut -d: -f 2`
		echo "Status is \"$checkStatus\""
	done
	if [[ "$checkStatus" != "$SUCCESS" ]];then
		echo -e "Error : notarization was refused, see the report:\n"
		xcrun notarytool info "$uuid" --apple-id "$notarizeUser" --password "$PASS" --team-id $teamId
		xcrun notarytool log "$uuid" --apple-id "$notarizeUser" --password "$PASS" --team-id $teamId
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
	rm -rf AudioDescriptorsPlugins.pkg
}

#==============================================================================
build_package
build_dmg
send_for_notarisation
wait_for_notarization
staple
# cleanup
