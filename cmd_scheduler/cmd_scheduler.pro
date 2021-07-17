TARGET = app_cmd_scheduler
TEMPLATE = app
QT -= gui core

VER_MAJ = 0
VER_MIN = 2
VER_PAT = 0
VERSION = $$VER_MAJ"."$$VER_MIN"."$$VER_PAT

!include ($$_PRO_FILE_PWD_/../common_config.pri) {
    message( "Not exist common_config.pri file." )
}

# for building
COMMON_LIB_ROOT=$$_PRO_FILE_PWD_/../common
COMM_LIB_ROOT=$$COMMON_LIB_ROOT/lib/communicator
SQLITE_LIB_ROOT=$$ROOT_PATH/$$BUILD_MODE/common/lib/sqlite
LIBS += -lpthread -lcommunicator -L$$COMM_LIB_ROOT/lib/$$CPU_ARCH -lsqlite3 -L$$SQLITE_LIB_ROOT/lib

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
    $$SQLITE_LIB_ROOT/include   \
    $$COMM_LIB_ROOT/include    \
    $$COMMON_LIB_ROOT    \
    $$COMMON_LIB_ROOT/lib/json    \
    $$COMMON_LIB_ROOT/lib/lock    \
    $$COMMON_LIB_ROOT/lib/logger  \
    $$COMMON_LIB_ROOT/lib/sys_sigslot    \
    $$COMMON_LIB_ROOT/lib/time    \
    $$COMMON_LIB_ROOT/lib/sqlite    \
    $$COMMON_LIB_ROOT/principle    \
    $$_PRO_FILE_PWD_/source/include


SOURCES += \
    $$files($$_PRO_FILE_PWD_/source/*.cpp)  \
    $$files($$COMMON_LIB_ROOT/principle/*.cpp)  \
    $$files($$COMMON_LIB_ROOT/CMDs/*.cpp)   \
    $$files($$COMMON_LIB_ROOT/lib/sqlite/*.cpp)


# for installation.
EXTRA_BINFILES = \
    $$_PRO_FILE_PWD_/$$TARGET

# PKG_CONFIG_DESCRIPTION=Common-communicator SDK library that support transaction-orient & service-orient.

!include ($$_PRO_FILE_PWD_/../deploy.pri) {
    message( "Not exist sdk_deploy.pri file." )
}

# # for process, if exist.
# include ($$_PRO_FILE_PWD_/post_proc.pri)
