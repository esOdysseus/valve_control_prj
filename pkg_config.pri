
# check variables
isEmpty(BUILD_MODE) {
    error("We need BUILD_MODE variable, please insert it.")
}

isEmpty(ROOT_PATH) {
    error("We need ROOT_PATH variable, please insert it.")
}

isEmpty(CPU_ARCH) {
    error("We need CPU_ARCH variable, please insert it.")
}

defineReplace(get_incs_pkgconfig){
    message("Called get_incs_pkgconfig")
    PKG_NAME = $$1
    equals(PKG_NAME, "automotive-dlt") {
        PKG_INCPATH = $$ROOT_PATH/$$BUILD_MODE/common/lib/dlt/include/dlt $$ROOT_PATH/$$BUILD_MODE/common/lib/dlt/include
    }

    equals(CPU_ARCH,"x86") {
        PKG_INCPATH = $$system(pkg-config --cflags-only-I $$PKG_NAME)
        PKG_INCPATH = $$replace(PKG_INCPATH, -I, )
    }

    message("PKG_INCPATH=$$PKG_INCPATH")
    return($$PKG_INCPATH)
}


defineReplace(get_libs_pkgconfig){
    message("Called get_libs_pkgconfig")
    PKG_NAME = $$1
    equals(PKG_NAME, "automotive-dlt") {
        PKG_LIBS = -L$$ROOT_PATH/$$BUILD_MODE/common/lib/dlt/lib -ldlt -lrt -lpthread
    }

    equals(CPU_ARCH,"x86") {
        PKG_LIBS = $$system(pkg-config --libs $$PKG_NAME)
    }

    message("PKG_LIBS=$$PKG_LIBS")
    return($$PKG_LIBS)
}

