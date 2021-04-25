# set variables for common configuration
CONFIG = c++11 -Wall

# check variables
isEmpty(BUILD_MODE) {
    error("We need BUILD_MODE variable, please insert it.")
}

ROOT_PATH=$(ROOT_PATH)
isEmpty(ROOT_PATH) {
    error("We need ROOT_PATH variable, please insert it.")
}
message( "[$$TARGET] BUILD_MODE=$$BUILD_MODE")
message( "[$$TARGET] ROOT_PATH=$$ROOT_PATH")

# set default FLAGS
QMAKE_CXXFLAGS += -ftemplate-depth=3000
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CFLAGS += -std=c11

CONFIG += $$BUILD_MODE
equals(BUILD_MODE, "debug") {
    QMAKE_CFLAGS += -g3
    QMAKE_CXXFLAGS += -g3
    CONFIG -= release
} else {
    QMAKE_CFLAGS += -Os
    QMAKE_CXXFLAGS += -Os
    CONFIG -= debug
}

# set cross-compiler & set additional FLAGS
!include ($$ROOT_PATH/env_for_cross_compile.pri) {
    error( "We need $$ROOT_PATH/env_for_cross_compile.pri file. Please check it." )
}

# define LOGGER-LEVEL variables
LOG_LEVEL_ERR = 1
LOG_LEVEL_WARN = 2
LOG_LEVEL_INFO = 3
LOG_LEVEL_DEBUG = 4

# set default DEFINITION
DEFINES += JSON_LIB_RAPIDJSON       # or JSON_LIB_HLOHMANN