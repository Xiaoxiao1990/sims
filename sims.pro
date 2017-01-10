TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    spi.c \
    sims.c \
    sim.c \
    functions.c \
    log.c \
    vsim.c \
    arbitrator.c

HEADERS += \
    types.h \
    sims.h \
    functions.h \
    spi.h \
    sim.h \
    log.h \
    vsim.h \
    arbi.h \
    arbitrator.h
