#if not defined(CORE_CONSTANTS_H)
#define CORE_CONSTANTS_H

#include <QtGlobal>

// Si on compile la DLL
#ifdef CORE_LIB
    #define CORE_EXPORT Q_DECL_EXPORT
#else
    #define CORE_EXPORT Q_DECL_IMPORT
#endif

#endif