#!/bin/bash
cd /var/home/ptomato/workspace/ns-android/test-app/runtime/.cxx/Debug/155f5n4c/x86
adb shell 'cd /data/local/tmp; rm libc++_shared.so libNativeScript.so TestNativeScriptException'
ls
adb push libc++_shared.so libNativeScript.so TestNativeScriptException /data/local/tmp
adb shell 'cd /data/local/tmp; LD_LIBRARY_PATH=$(pwd) ./TestNativeScriptException'
