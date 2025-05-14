#!/bin/bash

#
# Creates an app bundle for macOS – called by Makefile
#
#
# ./create-bundles.sh MIN_MACOS_VERSION APP_VERSION 
# ./create-bundles.sh 13.0 1.8.0
#


MIN_MACOS_VERSION=$1
VERSION=$2

APP=Tablecruncher
ICON=icon
BUILDDIR="./build/dist"
ASSETDIR="./assets"
APPDIR="${BUILDDIR}/$APP.app"
YEAR=$(date +%Y)


echo "Creating App Bundle for v$VERSION"

rm -rf ${APPDIR}

mkdir -p ${APPDIR}/Contents/Resources
mkdir -p ${APPDIR}/Contents/MacOS
echo APPLnone > ${APPDIR}/Contents/PkgInfo
cp ${BUILDDIR}/$APP ${APPDIR}/Contents/MacOS
chmod 755 ${APPDIR}/Contents/MacOS/$APP

iconutil -c icns -o ${APPDIR}/Contents/Resources/$ICON.icns ${ASSETDIR}/tablecruncher.iconset		# create iconset from directory

cat << EOF > ${APPDIR}/Contents/Info.plist
	<?xml version="1.0" encoding="UTF-8"?>
	<!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
	<plist version="0.9">
	<dict>
		<key>CFBundleInfoDictionaryVersion</key>
		<string>6.0</string>
		<key>NSHighResolutionCapable</key>
		<true/>
		<key>CFBundleName</key>
		<string>$APP</string>
		<key>CFBundlePackageType</key>
		<string>APPL</string>
		<key>CFBundleVersion</key>
		<string>$VERSION</string>
		<key>CFBundleShortVersionString</key>
		<string>$VERSION</string>
		<key>CFBundleIconFile</key>
		<string>$ICON</string>
		<key>CFBundleSignature</key>
		<string>none</string>
		<key>CFBundleDocumentTypes</key>
		<array>
			<dict>
				<key>CFBundleTypeExtensions</key>
				<array>
					<string>*</string>
					<string>csv</string>
				</array>
				<key>CFBundleTypeName</key>
				<string>AllTypes</string>
				<key>CFBundleTypeRole</key>
				<string>Editor</string>
				<key>CFBundleTypeOSTypes</key>
				<array>
					<string>****</string>
				</array>
			</dict>
		</array>
		<key>CFBundleExecutable</key>
		<string>Tablecruncher</string>
		<key>LSMinimumSystemVersion</key>
		<string>$MIN_MACOS_VERSION</string>
		<key>CFBundleIdentifier</key>
		<string>com.tablecruncher.Tablecruncher</string>
		<key>NSPrincipalClass</key>
		<string>NSApplication</string>
		<key>LSApplicationCategoryType</key>
		<string>public.app-category.utilities</string>
		<key>CFBundleSupportedPlatforms</key>
		<array>
			<string>MacOSX</string>
		</array>
		<key>NSHumanReadableCopyright</key>
		<string>Copyright © 2017-$YEAR Stefan Fischerländer. All rights reserved.</string>
	</dict>
	</plist>
EOF


