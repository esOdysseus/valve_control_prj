TARGET = app_valve_control
TEMPLATE = app
QT -= gui core

VER_MAJ = 0
VER_MIN = 1
VER_PAT = 0
VERSION = $$VER_MAJ"."$$VER_MIN"."$$VER_PAT

!include ($$_PRO_FILE_PWD_/../common_config.pri) {
    message( "Not exist common_config.pri file." )
}

# for building
EXTERNAL_LIB_ROOT=$$_PRO_FILE_PWD_/../lib/external_lib
LIBS += -lpthread -lcommunicator -L$$EXTERNAL_LIB_ROOT/lib/$$CPU_ARCH

DEFINES += LOGGER_TAG=\\\"APP\\\"
DEFINES += VER_MAJ=$$VER_MAJ
DEFINES += VER_MIN=$$VER_MIN
DEFINES += VER_PAT=$$VER_PAT

equals(BUILD_MODE, "debug") {
    DEFINES += LOG_TIME_ENABLE
    DEFINES += LOG_DEBUG_MODE
    DEFINES += LOG_LEVEL=$$LOG_LEVEL_DEBUG
}

equals(BUILD_MODE, "release") {
    DEFINES += LOG_LEVEL=$$LOG_LEVEL_INFO
}

INCLUDEPATH += \
    $$_PRO_FILE_PWD_/source/include    \
    $$EXTERNAL_LIB_ROOT/include    \
    $$ROOT_PATH/lib/exception   \
    $$ROOT_PATH/lib/json    \
    $$ROOT_PATH/lib/logger  \
    $$ROOT_PATH/lib/sys_sigslot

SOURCES += \
    $$files($$_PRO_FILE_PWD_/source/*.cpp)  \
    $$files($$ROOT_PATH/lib/json/*.cpp)

# for installation.
EXTRA_BINFILES = \
    $$_PRO_FILE_PWD_/$$TARGET

# PKG_CONFIG_DESCRIPTION=Common-communicator SDK library that support transaction-orient & service-orient.

!include ($$_PRO_FILE_PWD_/../deploy.pri) {
    message( "Not exist sdk_deploy.pri file." )
}

# # for process, if exist.
# include ($$_PRO_FILE_PWD_/post_proc.pri)
