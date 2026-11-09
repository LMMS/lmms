INCLUDE(BuildPlugin)
BUILD_PLUGIN(audiorecorder AudioRecorder.cpp AudioRecorderView.cpp
             MOCFILES AudioRecorder.h AudioRecorderView.h
             EMBEDDED_RESOURCES logo.png)
