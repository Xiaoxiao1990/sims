TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    spi.c \
    sims.c \
    sim.c \
    functions.c \
    arbitrator.c \
    log.c \
    vsim.c

HEADERS += \
    types.h \
    sims.h \
    functions.h \
    spi.h \
    sim.h \
    arbitrator.h \
    log.h \
    vsim.h
