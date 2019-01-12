// Copyright (c) 2018 Huawei.
// Author:  LI,Binfei (libinfei@huawei.com/179770346@qq.com)
//    v0.0.1 2019/01/03 11:59 
//  实现只依赖于ntdll的日志输出接口

#ifndef __HWPC_NATIVE_LOG_H_
#define __HWPC_NATIVE_LOG_H_

#include <stdio.h>
#include <stdarg.h>

#include "mini-printf.h"
#include "ntdll_interface.h"

enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

#define LOG_TRACE(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

static int g_level = LOG_DEBUG;
static const wchar_t * G_C_LOGFILE = L"\\??\\C:\\native.log";

void log_log(int level, const char *file, int line, const char *fmt, ...) {
    if (level < g_level) {
        return;
    }

    //Get current time
    LARGE_INTEGER t;
    NtQuerySystemTime(&t);
    TIME_FIELDS fields;
    RtlTimeToTimeFields(&t, &fields);

    //format
    va_list args;
    char bufall[512], buf[256];
    //first fmt the user input
    va_start(args, fmt);
    auto count = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    buf[count] = '\0';
    // second fmt with base info
    count = snprintf(bufall, sizeof(bufall), "[%d%d%d %d:%d:%d.%d] %d=> %s (%s:%d)\n", fields.Year, fields.Month,
        fields.Day, fields.Hour, fields.Minute, fields.Second, fields.Milliseconds, level, buf, file, line);
    bufall[count] = '\0';

    //write
    FILE_NETWORK_OPEN_INFORMATION aInfo;
    OBJECT_ATTRIBUTES aFileAttrib = { 0 };
    UNICODE_STRING subprocesspath;
    RtlInitUnicodeString(&subprocesspath, G_C_LOGFILE);
    InitializeObjectAttributes(&aFileAttrib, &subprocesspath, OBJ_CASE_INSENSITIVE, NULL, NULL);
    HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    const int allocSize = 2048;
    LARGE_INTEGER largeInteger;
    largeInteger.QuadPart = allocSize;
    auto status = NtCreateFile(&hFile, FILE_APPEND_DATA, &aFileAttrib, &ioStatusBlock, &largeInteger,
        FILE_ATTRIBUTE_NORMAL, FILE_WRITE_ACCESS, FILE_OPEN | FILE_CREATE, FILE_NON_DIRECTORY_FILE, NULL, 0);
    if (status == STATUS_SUCCESS && hFile != NULL) {
        LARGE_INTEGER liByteOffset = { 0 };
        status = NtWriteFile(hFile, NULL, NULL, NULL, &ioStatusBlock, (PVOID)(bufall), count, &liByteOffset, NULL);
        if (status == STATUS_SUCCESS || status == 0x00000103) {
            DbgPrint("[NATIVE] write file success.");
            return;
        }
    }
}

#endif //! __HWPC_NATIVE_LOG_H_
