#include "Log.h"

// GUID: 6578589E-36D5-489B-B572-EB707C4724A9

TRACELOGGING_DEFINE_PROVIDER(g_hLoggingProvider, LOG_PROVIDER_NAME,
    (0x6578589e, 0x36d5, 0x489b, 0xb5, 0x72, 0xeb, 0x70, 0x7c, 0x47, 0x24, 0xa9));

NTSTATUS
LogSessionStart()
{
    return TraceLoggingRegister( g_hLoggingProvider );
}

VOID
LogMessage(
    _In_ CONST PWCHAR Format,
    _In_ ...
    )
{
    va_list args;
    WCHAR msgBuff[LOG_MSG_MAX_LEN + 1] = { 0 };

    va_start(args, Format);

    vswprintf_s(
        (PWCHAR)&msgBuff,
        LOG_MSG_MAX_LEN,
        Format,
        args
        );

    va_end(args);

    TraceLoggingWrite(
        g_hLoggingProvider,
        "RuntimeMessage",
        TraceLoggingWideString(msgBuff)
        );
}

VOID
LogSessionEnd()
{
    TraceLoggingUnregister( g_hLoggingProvider );
}
