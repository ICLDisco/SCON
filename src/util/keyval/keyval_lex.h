/* -*- C -*-
 *
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 # Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef SCON_UTIL_KEYVAL_LEX_H_
#define SCON_UTIL_KEYVAL_LEX_H_

#include <src/include/scon_config.h>

#ifdef malloc
#undef malloc
#endif
#ifdef realloc
#undef realloc
#endif
#ifdef free
#undef free
#endif

#include <stdio.h>

int scon_util_keyval_yylex(void);
int scon_util_keyval_init_buffer(FILE *file);
int scon_util_keyval_yylex_destroy(void);

extern FILE *scon_util_keyval_yyin;
extern bool scon_util_keyval_parse_done;
extern char *scon_util_keyval_yytext;
extern int scon_util_keyval_yynewlines;
extern int scon_util_keyval_yylineno;

/*
 * Make lex-generated files not issue compiler warnings
 */
#define YY_STACK_USED 0
#define YY_ALWAYS_INTERACTIVE 0
#define YY_NEVER_INTERACTIVE 0
#define YY_MAIN 0
#define YY_NO_UNPUT 1
#define YY_SKIP_YYWRAP 1

enum scon_keyval_parse_state_t {
    SCON_UTIL_KEYVAL_PARSE_DONE,
    SCON_UTIL_KEYVAL_PARSE_ERROR,

    SCON_UTIL_KEYVAL_PARSE_NEWLINE,
    SCON_UTIL_KEYVAL_PARSE_EQUAL,
    SCON_UTIL_KEYVAL_PARSE_SINGLE_WORD,
    SCON_UTIL_KEYVAL_PARSE_VALUE,
    SCON_UTIL_KEYVAL_PARSE_MCAVAR,
    SCON_UTIL_KEYVAL_PARSE_ENVVAR,
    SCON_UTIL_KEYVAL_PARSE_ENVEQL,

    SCON_UTIL_KEYVAL_PARSE_MAX
};
typedef enum scon_keyval_parse_state_t scon_keyval_parse_state_t;

#endif
