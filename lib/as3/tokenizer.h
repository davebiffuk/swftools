/* tokenizer.h

   Copyright (c) 2008 Matthias Kramm <kramm@quiss.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef __parser_h__
#define __parser_h__

#include "../q.h"
#include "abc.h"
#include "pool.h"
#include "files.h"
#include "tokenizer.h"
#include "registry.h"
#include "code.h"
#include "opcodes.h"

DECLARE(token);
DECLARE_LIST(token);
DECLARE(param);
DECLARE_LIST(param);
DECLARE(params);
DECLARE(typedcode);
DECLARE_LIST(typedcode);

struct _param {
    char*name;
    classinfo_t*type;
    constant_t*value;
};

struct _params {
    param_list_t*list;
    char varargs;
};

struct _typedcode {
    code_t*c;
    classinfo_t*t;
};

/* small helper structs: */
typedef struct _codeandnumber {
    code_t*cc;
    int len;
} codeandnumber_t;
typedef struct _for_start {
    char*name;
    char each;
} for_start_t;
typedef struct _regexp {
    char*pattern;
    char*options;
} regexp_t;

extern char start_of_expression;

typedef token_t*tokenptr_t;

#include "parser.tab.h"

extern int as3_verbosity;
extern int as3_pass;
#define syntaxerror as3_error
void as3_error(const char*format, ...);
void as3_warning(const char*format, ...);
void as3_softwarning(const char*format, ...);

#define T_EOF 0

extern int avm2_lex();
extern int avm2_lex_destroy();

#endif
