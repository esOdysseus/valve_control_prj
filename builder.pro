TEMPLATE = subdirs
CONFIG += ordered

equals(TARGET, "valve_controller") {
    SUBDIRS += valve_controller
}

equals(TARGET, "cmd_scheduler") {
    SUBDIRS += cmd_scheduler
}

DISTFILES += \