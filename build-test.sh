#!/bin/bash
ndk-build
adb -e shell "su -c rm /data/fbstream"
adb push libs/armeabi/fbstream /sdcard/
adb -e shell "su -c \"cp /sdcard/fbstream /data/ && chmod 755 /data/fbstream\""
adb shell
#&& /data/fbstream"
