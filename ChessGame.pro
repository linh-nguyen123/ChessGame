# Khai bao QT modules dung mot lan duy nhat
QT += core gui multimedia sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    chessbox.cpp \
    chesspiece.cpp \
    databasemanager.cpp \
    game.cpp \
    main.cpp \
    mainwindow.cpp \
    stockfishmanager.cpp

HEADERS += \
    chessbox.h \
    chesspiece.h \
    databasemanager.h \
    game.h \
    mainwindow.h \
    stockfishmanager.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    res.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# SDL2
DEFINES += SDL_MAIN_HANDLED
DEFINES += SDL_DISABLE_IMMINTRIN_H

INCLUDEPATH += $$PWD/libs/SDL2/include/SDL2
INCLUDEPATH += $$PWD/libs/SDL2_mixer/include/SDL2

LIBS += -L$$PWD/libs/SDL2/lib -lmingw32 -lSDL2
LIBS += -L$$PWD/libs/SDL2_mixer/lib -lSDL2_mixer
