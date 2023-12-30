#!/bin/bash

PACKAGE_NAME="Dragonfly_X2.pkg"
BUNDLE_NAME="org.rti-zone.DragonflyX2"

if [ ! -z "$app_id_signature" ]; then
    codesign -f -s "$app_id_signature" --verbose ../build/Release/libDragonfly.dylib
fi

mkdir -p ROOT/tmp/Dragonfly_X2/
cp "../domelist Dragonfly.txt" ROOT/tmp/Dragonfly_X2/
cp "../Dragonfly.ui" ROOT/tmp/Dragonfly_X2/
cp "../Lunatico2.png" ROOT/tmp/Dragonfly_X2/
cp "../build/Release/libDragonfly.dylib" ROOT/tmp/Dragonfly_X2/


if [ ! -z "$installer_signature" ]; then
	# signed package using env variable installer_signature
	pkgbuild --root ROOT --identifier $BUNDLE_NAME --sign "$installer_signature" --scripts Scripts --version 1.0 $PACKAGE_NAME
	pkgutil --check-signature ./${PACKAGE_NAME}
else
	pkgbuild --root ROOT --identifier $BUNDLE_NAME --scripts Scripts --version 1.0 $PACKAGE_NAME
fi

rm -rf ROOT
