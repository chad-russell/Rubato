# Add more folders to ship with the application, here
folder_01.source = qml/Rubato
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp cqt.cpp \
    model.cpp \
    frequencyimageprovider.cpp \
    phasevocoder.cpp

HEADERS += i_indices.h j_indices.h real_vals.h imag_vals.h \
    model.h \
    frequencyimageprovider.h \
    phasevocoder.h

QMAKE_CXXFLAGS += -std=c++0x
CONFIG += c++11

LIBS += -L/usr/local/lib -lfftw3 -lsndfile -lsamplerate -lmpg123

QT += multimedia

# Installation path
# target.path =

# Please do not modify the following two lines. Required for deployment.
include(qtquick2applicationviewer/qtquick2applicationviewer.pri)
qtcAddDeployment()
