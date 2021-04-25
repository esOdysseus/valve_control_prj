isEmpty(CPU_ARCH) {
    error("We need CPU_ARCH variable, please insert it.")
}

# Check & Set Cross-Compiler.
CHECK_ENV=$$(CC)
isEmpty( CHECK_ENV ) {
    equals(CPU_ARCH,"x86") {
        message( "Set default-compiler for x86." )
    } else {
        error( "Can not exist cross-compiler for $$CPU_ARCH." )
    }
} else {
    CHECK_ENV=$$(CXX)
    isEmpty( CHECK_ENV ) {
        error("We need Environment CXX, please insert value of variable.")
    }

    CHECK_ENV=$$(LINK)
    isEmpty( CHECK_ENV ) {
        error("We need Environment LINK, please insert value of variable.")
    }

    # Set Cross-Compiler according to exported-ENV
    # QT_INSTALL_PREFIX       = 
    QMAKE_CC                = $$(CC)
    QMAKE_CXX               = $$(CXX)
    QMAKE_LINK              = $$(LINK)
    # QMAKE_LINK_SHLIB        = 
    # QMAKE_AR                = 
    # QMAKE_OBJCOPY           = 
    # QMAKE_STRIP             = 

    message( "QMAKE_CC=$$QMAKE_CC" )
    message( "QMAKE_CXX=$$QMAKE_CXX" )
    message( "QMAKE_LINK=$$QMAKE_LINK" )
}


# Set CFLAG/CXXFLAG/DEFINES
equals(CPU_ARCH,"x86") {
    message( "Set CFLAG or CXXFLAG for x86." )
} else {
    equals(CPU_ARCH, "armv7") {
        message( "Set CFLAG or CXXFLAG for armv7." )
        QMAKE_CFLAGS -= -m64 -m32
        QMAKE_CXXFLAGS -= -m64 -m32
        QMAKE_LFLAGS -= -m64 -m32
    } else {
        equals(CPU_ARCH, "aarch64") {
            message( "Set CFLAG or CXXFLAG for aarch64." )
            QMAKE_CFLAGS -= -m64 -m32
            QMAKE_CXXFLAGS -= -m64 -m32
            QMAKE_LFLAGS -= -m64 -m32
        } else {
            error("Not supported CPU_ARCH.($${CPU_ARCH})")
        }
    }
}
