include(../../libs.pri)

TEMPLATE = lib
TARGET = $$ROOT/lib/coquillo_core
DEPENDPATH += .
INCLUDEPATH += .

LIBS += -ltag

# Input
HEADERS += globals.h \
           MediaScanner.h \
           MetaData.h \
           MetaDataImage.h \
           MetaDataModel2.h \
           MetaDataReader.h \
           MetaDataWriter.h \
					 ModelDataInspector.h \

SOURCES += globals.cpp \
           MediaScanner.cpp \
           MetaData.cpp \
           MetaDataImage.cpp \
           MetaDataModel2.cpp \
           MetaDataReader.cpp \
           MetaDataWriter.cpp \
					 ModelDataInspector.cpp \
