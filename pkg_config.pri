
defineReplace(get_incs_pkgconfig){
    message("Called get_incs_pkgconfig")
    PKG_NAME = $$1
    PKG_INCPATH = $$system(pkg-config --cflags-only-I $$PKG_NAME)
    PKG_INCPATH = $$replace(PKG_INCPATH, -I, )

    message("PKG_INCPATH=$$PKG_INCPATH")
    return($$PKG_INCPATH)
}


defineReplace(get_libs_pkgconfig){
    message("Called get_libs_pkgconfig")
    PKG_NAME = $$1
    PKG_LIBS = $$system(pkg-config --libs $$PKG_NAME)

    message("PKG_LIBS=$$PKG_LIBS")
    return($$PKG_LIBS)
}

