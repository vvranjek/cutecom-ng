#!/bin/bash

echo
echo Running before_deploy-osx
echo

export RELEASE_PKG_FILE="./bin/serial-ninja.${TRAVIS_TAG}.dmg"
hdiutil create "${RELEASE_PKG_FILE}" -volname "Cutecom-ng" -srcfolder ./bin
