#!/bin/bash
ndk-build
#adb -e shell "su -c rm /data/fbstream"
adb push libs/armeabi/fbstream /data/broodlord/
adb -e shell "su -c chmod 755 /data/broodlord/fbstream"
adb shell
#&& /data/fbstream"
