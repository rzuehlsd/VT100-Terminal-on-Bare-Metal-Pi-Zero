#ifndef _DEBUG_LEVELS_H_
#define _DEBUG_LEVELS_H_

// Bitmap-based debug severity levels
#define LOG_ERROR_BIT    0x01
#define LOG_WARNING_BIT  0x02
#define LOG_NOTICE_BIT   0x04
#define LOG_DEBUG_BIT    0x08

// Legacy compatibility defines for USPI and other code
// USPI uses: LOG_ERROR=1, LOG_WARNING=2, LOG_NOTICE=3, LOG_DEBUG=4
// Our LogWrite() function automatically converts these to bitmap values

// Global debug severity variable (can be changed at runtime)
extern unsigned g_debug_severity;

// Function to set debug severity level
extern void SetDebugSeverity(unsigned severity);

// Function to get current debug severity level
extern unsigned GetDebugSeverity(void);

// Macro to check if a severity level should be logged
#define SHOULD_LOG(severity) ((severity) & g_debug_severity)

// Simplified logging macros - no need to specify severity in calls
#define LogError(source, ...) \
    do { if (SHOULD_LOG(LOG_ERROR_BIT)) LogWriteInternal((source), LOG_ERROR_BIT, __FILE__, __LINE__, __VA_ARGS__); } while(0)

#define LogWarning(source, ...) \
    do { if (SHOULD_LOG(LOG_WARNING_BIT)) LogWriteInternal((source), LOG_WARNING_BIT, NULL, 0, __VA_ARGS__); } while(0)

#define LogNotice(source, ...) \
    do { if (SHOULD_LOG(LOG_NOTICE_BIT)) LogWriteInternal((source), LOG_NOTICE_BIT, NULL, 0, __VA_ARGS__); } while(0)

#define LogDebug(source, ...) \
    do { if (SHOULD_LOG(LOG_DEBUG_BIT)) LogWriteInternal((source), LOG_DEBUG_BIT, __FILE__, __LINE__, __VA_ARGS__); } while(0)

#endif
