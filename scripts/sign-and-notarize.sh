#!/bin/bash

APP_NAME=Tablecruncher
VERSION=$1
APP=${APP_NAME}.app

LIPO=`lipo -info ${APP}/Contents/MacOS/${APP_NAME}`
if [[ $LIPO == *"arm64"* ]]; then
    ARCH=arm64
elif [[ $LIPO == *"x86_64"* ]]; then
    ARCH=x86_64
fi
ZIP=Tablecruncher-${VERSION}-${ARCH}.zip

echo "Signing and notarizing $ZIP"

codesign --deep --options=runtime --sign "Developer ID Application: Stefan Fischerlaender" --timestamp $APP
codesign -vvv --deep --strict $APP

/usr/bin/ditto -c -k --keepParent $APP $ZIP
xcrun notarytool submit $ZIP --progress --apple-id "developer@fischerlaender.de" --team-id QUD74JYY93 --wait

rm $ZIP
xcrun stapler staple $APP

spctl -a -v $APP

/usr/bin/ditto -c -k --keepParent $APP $ZIP
