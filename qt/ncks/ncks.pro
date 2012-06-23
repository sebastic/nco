# template type is application
TEMPLATE = app
# name
TARGET = ncks

CONFIG -= qt
CONFIG += debug_and_release

#nco library
CONFIG( debug, debug|release ) {
    # debug
        win32:LIBS += ../libnco/debug/libnco.lib
        unix:LIBS  += ../libnco/debug/liblibnco.a
} else {
    # release
        win32:LIBS += ../libnco/release/libnco.lib
        unix:LIBS  += ../libnco/release/liblibnco.a
}

#nco avoid writing temporary file
win32:DEFINES += WRT_TMP_FL

#netCDF
DEFINES += ENABLE_NETCDF4
DEFINES += HAVE_NETCDF4_H

HEADERS   = 
SOURCES   = ../../src/nco/ncks.c


# netCDF library
# gcc settings to use C99
# HDF5 and netCDF LIB order is important
# _BSD_SOURCE and _POSIX_SOURCE needed
unix {
 DEFINES += HAVE_CONFIG_H
 INCLUDEPATH += ../../
 DEFINES += _BSD_SOURCE
 DEFINES += _POSIX_SOURCE
 QMAKE_CFLAGS += -std=c99
 INCLUDEPATH += /usr/local/include
 INCLUDEPATH += /usr/local

 LIBS += /usr/local/lib/libnetcdf.a
 LIBS += /usr/local/lib/libhdf5_hl.a
 LIBS += /usr/local/lib/libhdf5.a
 LIBS += /usr/lib/x86_64-linux-gnu/libz.a


 LIBS += ~/libs/install/udunits-2.1.24/lib/libudunits2.a
 LIBS += ~/libs/install/expat-2.1.0/lib/libexpat.a


}
win32 {
 INCLUDEPATH += $(HEADER_NETCDF)
 LIBS += $(LIB_NETCDF)
 LIBS += $(LIB_DISPATCH)

 
 LIBS += $(LIB_NETCDF4)
 LIBS += $(LIB_HDF5)
 LIBS += $(LIB_HDF5_HL)
 LIBS += $(LIB_ZLIB)
 LIBS += $(LIB_SZIP)
 
 DEFINES += _CRT_SECURE_NO_WARNINGS
 DEFINES += _CRT_NONSTDC_NO_DEPRECATE
 CONFIG += console


}
