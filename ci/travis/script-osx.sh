#!/bin/bash

echo
echo Running script-osx.sh...
echo

/usr/local/opt/qt5/bin/qmake -config release
/usr/local/opt/qt5/bin/qmake -v
make
/usr/local/opt/qt5/bin/macdeployqt bin/serial-ninja.app -always-overwrite -verbose=2
wget https://raw.githubusercontent.com/aurelien-rainone/macdeployqtfix/master/macdeployqtfix.py
python macdeployqtfix.py bin/serial-ninja.app/Contents/MacOS/serial-ninja /usr/local/Cellar/qt5/5.5.0_1/
