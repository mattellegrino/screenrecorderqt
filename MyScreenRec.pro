QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SHA1 = $$system(git rev-parse --short=8 HEAD)
DEFINES += "GITSHA1=\\\"$$SHA1\\\""

SOURCES += \
    getRecArea.cpp \
    main.cpp \
    mainwindow.cpp \
    screen-rec-apis/src/ScreenRecorder.cpp \
    screen-rec-apis/src/audio.cpp \
    screen-rec-apis/src/video.cpp

HEADERS += \
    getRecArea.h \
    mainwindow.h \
    screen-rec-apis/include/ListAVDevices.h \
    screen-rec-apis/include/ScreenRecorder.h \
    screen-rec-apis/include/ffmpeglib.h \
    screen-rec-apis/include/outputColors.h \
    screen-rec-apis/include/recordStatus.h

FORMS += \
    getRecArea.ui \
    mainwindow.ui

TRANSLATIONS += \
    MyScreenRec_en_150.ts


INCLUDEPATH += /usr/include/x86_64-linux-gnu/libavcodec
INCLUDEPATH += "C:/ffmpeg/include"

unix: LIBS +=  -lavformat  -lavcodec -lavutil -lavfilter -lswscale -lavutil -lavdevice -lswresample
win32: LIBS += -LC:/ffmpeg/lib/ -lavformat -lavcodec -lavutil -lavfilter -lswscale -lavutil -lavdevice -lswresample



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
