# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
    phasevocoder.cpp

HEADERS += \
    phasevocoder.h

QMAKE_CXXFLAGS += -std=c++0x
CONFIG += c++11

LIBS += -L/usr/local/lib -lfftw3 -lsndfile -lsamplerate -lmpg123
