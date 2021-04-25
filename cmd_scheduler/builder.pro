TEMPLATE = subdirs
CONFIG += ordered

equals(TARGET, "cmd_scheduler") {
    SUBDIRS += cmd_scheduler
}

DISTFILES += \