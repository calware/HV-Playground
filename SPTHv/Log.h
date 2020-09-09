#ifndef __LOG_H__
#define __LOG_H__

// GUID: 6578589E-36D5-489B-B572-EB707C4724A9

#include <wdm.h>
#include <TraceLoggingProvider.h>

#include <stdio.h>
#include <stdarg.h>

#define LOG_MSG_MAX_LEN sizeof(WCHAR) * 500

#define LOG_PROVIDER_NAME "SPTHv"

TRACELOGGING_DECLARE_PROVIDER(g_hLoggingProvider);

NTSTATUS
LogSessionStart();

VOID
LogMessage(
    _In_ CONST PWCHAR Format,
    _In_ ...
    );

VOID
LogSessionEnd();

#endif // __LOG_H__
