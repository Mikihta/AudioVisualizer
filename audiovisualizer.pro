QT += core gui multimedia widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AudioVisualizer
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    audiocapture.cpp \
    spectrumanalyzer.cpp

HEADERS += \
    mainwindow.h \
    audiocapture.h \
    spectrumanalyzer.h

FORMS += \
    mainwindow.ui

LIBS += -lole32 -lwinmm

CONFIG += c++11
