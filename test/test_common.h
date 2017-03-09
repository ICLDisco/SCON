
/**
 * Copyright (c) 2017 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "scon_config.h"
#include "scon.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>

#include "src/util/argv.h"

#define TEST_JOBNAME "scon_test"
#define TEST_CREDENTIAL "dummy"

#define OUTPUT_MAX 1024
/* WARNING: scon_test_output_prepare is currently not threadsafe!
 * fix it once needed!
 */
char *scon_test_output_prepare(const char *fmt,... )
{
    static char output[OUTPUT_MAX];
    va_list args;
    va_start( args, fmt );
    memset(output, 0, sizeof(output));
    vsnprintf(output, OUTPUT_MAX - 1, fmt, args);
    va_end(args);
    return output;
}
int scon_test_verbose = 0;
FILE *file;

#define STRIPPED_FILE_NAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define TEST_OUTPUT(x) { \
fprintf(stdout, "%s:%s: %s\n",STRIPPED_FILE_NAME, __func__, \
        scon_test_output_prepare x ); \
fflush(stdout); \
}

// Write output wightout adding anything to it.
// Need for automate tests to receive "OK" string
#define TEST_OUTPUT_CLEAR(x) { \
fprintf(file, "%s", scon_test_output_prepare x ); \
fflush(file); \
}

// Always write errors to the stderr
#define TEST_ERROR(x) { \
fprintf(stderr,"ERROR [%s:%d:%s]: %s\n", STRIPPED_FILE_NAME, __LINE__, __func__, \
        scon_test_output_prepare x ); \
fflush(stderr); \
}

#define TEST_VERBOSE_ON() (scon_test_verbose = 1)
#define TEST_VERBOSE_GET() (scon_test_verbose)

#define TEST_VERBOSE(x) { \
    if( scon_test_verbose ){ \
    TEST_OUTPUT(x); \
} \
}

#define TEST_DEFAULT_TIMEOUT 10
#define MAX_DIGIT_LEN 10


#define TEST_SET_FILE(prefix, ns_id, rank) { \
char *fname = malloc( strlen(prefix) + MAX_DIGIT_LEN + 2 ); \
              sprintf(fname, "%s.%d.%d", prefix, ns_id, rank); \
              file = fopen(fname, "w"); \
                     free(fname); \
    if( NULL == file ){ \
    fprintf(stderr, "Cannot open file %s for writing!", fname); \
    exit(1); \
} \
}

#define TEST_CLOSE_FILE() { \
    if ( stderr != file ) { \
    fclose(file); \
} \
}

#endif
