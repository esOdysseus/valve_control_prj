TEMPLATE = subdirs
CONFIG += ordered

equals(TARGET, "valve_control") {
    SUBDIRS += valve_control
}

DISTFILES += \