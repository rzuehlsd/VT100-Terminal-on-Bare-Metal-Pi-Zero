#ifndef _EE_PRINTF_H_
#define _EE_PRINTF_H_

#include "debug_levels.h"

extern void ee_printf(const char *fmt, ...);

// Legacy LogWrite function for backward compatibility
extern void LogWrite (const char *pSource,		// short name of module
        	       unsigned	   Severity,		// see debug_levels.h
	       const char *pMessage, ...);	// uses printf format options

// Internal LogWrite function used by simplified macros
extern void LogWriteInternal (unsigned Severity,        // see debug_levels.h  
                              const char *pFile,        // source file name (__FILE__ or NULL)
                              int nLine,                // source line number (__LINE__ or 0)
                              const char *pMessage, ...); // uses printf format options

#endif
