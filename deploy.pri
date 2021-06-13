# Installation
CONFIG += create_pc create_prl no_install_prl

message( "[$$TARGET] DESTDIR=$$DESTDIR")

# create variable for pkg-config & installation.
target.path = $$DESTDIR
target.files = &&EXTRA_BINFILES

# set variable for installation.
INSTALLS += target

