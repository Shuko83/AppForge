#pragma once

#include <QtGlobal>

// Si on compile la DLL
#ifdef CORE_LIB
    #define CORE_EXPORT Q_DECL_EXPORT
#else
    #define CORE_EXPORT Q_DECL_IMPORT
#endif
