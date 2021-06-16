/***
 * version.h
 * Copyright [2021-] 
 * Written by EunSeok Kim <es.odysseus@gmail.com>
 */
#ifndef _VERSION_INFO_H_
#define _VERSION_INFO_H_

#include <string>

// for APP-Version info.
#if !defined( VER_MAJ )
    #error "We need VER_MAJ for APP-version."
#endif

#if !defined( VER_MIN )
    #error "We need VER_MIN for APP-version."
#endif

#if !defined( VER_PAT )
    #error "We need VER_PAT for APP-version."
#endif

#define STRING_OF_APP_VERSION      (std::to_string(VER_MAJ) + '.' + std::to_string(VER_MIN) + '.' + std::to_string(VER_PAT))



#endif // _VERSION_INFO_H_