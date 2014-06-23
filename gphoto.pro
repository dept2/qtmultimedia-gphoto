TARGET = gphoto

QT       += core gui multimedia
TEMPLATE =  lib
CONFIG   += plugin

PLUGIN_TYPE=mediaservice
PLUGIN_CLASS_NAME=GPhotoServicePlugin

#DESTDIR = $$[QT_INSTALL_PLUGINS]/mediaservice

SOURCES += gphotoserviceplugin.cpp \
    gphotomediaservice.cpp \
    gphotocamerasession.cpp \
    gphotocameracontrol.cpp \
    gphotovideorenderercontrol.cpp \
    gphotocameraworker.cpp \
    gphotocameraimagecapturecontrol.cpp \
    gphotocameracapturedestinationcontrol.cpp
HEADERS += gphotoserviceplugin.h \
    gphotomediaservice.h \
    gphotocamerasession.h \
    gphotocameracontrol.h \
    gphotovideorenderercontrol.h \
    gphotocameraworker.h \
    gphotocameraimagecapturecontrol.h \
    gphotocameracapturedestinationcontrol.h
OTHER_FILES += gphoto.json
LIBS += -lgphoto2

target.path = $$[QT_INSTALL_PLUGINS]/mediaservice
INSTALLS += target
