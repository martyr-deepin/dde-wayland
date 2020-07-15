TARGET = dde-wayland-client
TEMPLATE = lib

QT += waylandclient gui-private
# for QTBUG-63384
QT.waylandclient.uses -= wayland-client wayland-cursor

CONFIG += wayland-scanner
CONFIG += c++11 create_pc create_prl no_install_prl

WAYLANDCLIENTSOURCES += \
    ../protocol/dde-shell.xml

HEADERS += \
    dshellsurface.h

SOURCES += \
    dshellsurface.cpp

isEmpty(VERSION): VERSION = 0.0.1
isEmpty(PREFIX): PREFIX = /usr

isEmpty(LIB_INSTALL_DIR) {
    target.path = $$PREFIX/lib
} else {
    target.path = $$LIB_INSTALL_DIR
}

isEmpty(INCLUDE_INSTALL_DIR) {
    includes.path = $$PREFIX/include/$$TARGET
} else {
    includes.path = $$INCLUDE_INSTALL_DIR
}

includes.files += \
    $$PWD/dshellsurface.h

QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_VERSION = $$VERSION
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_NAME = $$TARGET
QMAKE_PKGCONFIG_DESCRIPTION = Wayland client library of DDE
QMAKE_PKGCONFIG_INCDIR = $$includes.path

INSTALLS += target includes
