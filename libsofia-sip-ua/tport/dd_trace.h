#ifndef DD_TRACE_H
#define DD_TRACE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#else
#include <unistd.h> 
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#ifdef _MSC_VER
#pragma warning(push) 
#pragma warning(disable: 4996)
#endif



    static void dd_vtrace(const char* filename, const char* format, va_list arg)
    {
        int r = -1;
        va_list arg_copy;
        va_copy(arg_copy, arg);

        r = vsnprintf(0, 0, format, arg);
        if (r > 0) {
            r += 1;
            char* buf = (char*)malloc(r);
            if (buf) {
                r = vsnprintf(buf, r, format, arg_copy);
                if (r > 0) {
                    int fd = open(filename, O_RDWR | O_APPEND | O_CREAT | O_BINARY, S_IRUSR | S_IWUSR);
                    if (fd >= 0) {
                        r = write(fd, buf, r);
                        close(fd);
                    }
                }
                free(buf);
            }
        }
        va_end(arg_copy);

    }

    static void dd_trace(const char* filename, const char* format, ...)
    {
        va_list arg;
        va_start(arg, format);
        dd_vtrace(filename, format, arg);
        va_end(arg);
    }



#ifdef _MSC_VER
#pragma warning(pop)
#endif



#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif