#pragma once

#include <QtGlobal>

// Si on compile la DLL
#ifdef AFCORE_LIB
    #define AFCORE_EXPORT Q_DECL_EXPORT
#else
    #define AFCORE_EXPORT Q_DECL_IMPORT
#endif
