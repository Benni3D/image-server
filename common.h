#ifndef FILE_COMMON_H
#define FILE_COMMON_H

#ifndef PROG
#error PROG is undefined
#endif

#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

struct login_packet {
   uint32_t width, height;
};

static void error(const char* msg, ...) {
   va_list ap;
   va_start(ap, msg);
   fputs(PROG ": ", stderr);
   vfprintf(stderr, msg, ap);
   fputc('\n', stderr);
}
static const char* geterr(void) { return strerror(errno); }


#endif /* FILE_COMMON_H */
