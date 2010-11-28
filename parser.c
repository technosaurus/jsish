/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 1



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     STRING = 258,
     IDENTIFIER = 259,
     IF = 260,
     ELSE = 261,
     FOR = 262,
     IN = 263,
     WHILE = 264,
     DO = 265,
     CONTINUE = 266,
     SWITCH = 267,
     CASE = 268,
     DEFAULT = 269,
     BREAK = 270,
     FUNC = 271,
     RETURN = 272,
     LOCAL = 273,
     NEW = 274,
     DELETE = 275,
     TRY = 276,
     CATCH = 277,
     FINALLY = 278,
     THROW = 279,
     WITH = 280,
     UNDEF = 281,
     _TRUE = 282,
     _FALSE = 283,
     _THIS = 284,
     ARGUMENTS = 285,
     FNUMBER = 286,
     REGEXP = 287,
     __DEBUG = 288,
     MIN_PRI = 289,
     ARGCOMMA = 290,
     DIVAS = 291,
     BXORAS = 292,
     BORAS = 293,
     BANDAS = 294,
     URSHFAS = 295,
     RSHFAS = 296,
     LSHFAS = 297,
     MODAS = 298,
     MULAS = 299,
     MNSAS = 300,
     ADDAS = 301,
     OR = 302,
     AND = 303,
     NNEQ = 304,
     EEQU = 305,
     NEQ = 306,
     EQU = 307,
     INSTANCEOF = 308,
     GEQ = 309,
     LEQ = 310,
     URSHF = 311,
     RSHF = 312,
     LSHF = 313,
     VOID = 314,
     TYPEOF = 315,
     DEC = 316,
     INC = 317,
     NEG = 318,
     MAX_PRI = 319
   };
#endif
/* Tokens.  */
#define STRING 258
#define IDENTIFIER 259
#define IF 260
#define ELSE 261
#define FOR 262
#define IN 263
#define WHILE 264
#define DO 265
#define CONTINUE 266
#define SWITCH 267
#define CASE 268
#define DEFAULT 269
#define BREAK 270
#define FUNC 271
#define RETURN 272
#define LOCAL 273
#define NEW 274
#define DELETE 275
#define TRY 276
#define CATCH 277
#define FINALLY 278
#define THROW 279
#define WITH 280
#define UNDEF 281
#define _TRUE 282
#define _FALSE 283
#define _THIS 284
#define ARGUMENTS 285
#define FNUMBER 286
#define REGEXP 287
#define __DEBUG 288
#define MIN_PRI 289
#define ARGCOMMA 290
#define DIVAS 291
#define BXORAS 292
#define BORAS 293
#define BANDAS 294
#define URSHFAS 295
#define RSHFAS 296
#define LSHFAS 297
#define MODAS 298
#define MULAS 299
#define MNSAS 300
#define ADDAS 301
#define OR 302
#define AND 303
#define NNEQ 304
#define EEQU 305
#define NEQ 306
#define EQU 307
#define INSTANCEOF 308
#define GEQ 309
#define LEQ 310
#define URSHF 311
#define RSHF 312
#define LSHF 313
#define VOID 314
#define TYPEOF 315
#define DEC 316
#define INC 317
#define NEG 318
#define MAX_PRI 319




/* Copy the first part of user declarations.  */
#line 1 "parser.y"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "code.h"
#include "pstate.h"
#include "func.h"
#include "scope.h"
#include "lexer.h"
#include "parser.h"
#include "unichar.h"

typedef struct ArgList {
	unichar *argname;
	struct ArgList *tail;
	struct ArgList *next;
} ArgList;

static ArgList *arglist_new(const unichar *name)
{
	ArgList *a = malloc(sizeof(ArgList));
	memset(a, 0, sizeof(ArgList));
	a->argname = unistrdup(name);
	a->tail = a;
	return a;
}

static ArgList *arglist_insert(ArgList *a, const unichar *name)
{
	ArgList *b = malloc(sizeof(ArgList));
	memset(b, 0, sizeof(ArgList));
	b->argname = unistrdup(name);
	a->tail->next = b;
	a->tail = b;
	return a;
}

typedef struct ForinVar {
	unichar *varname;
	OpCodes *local;
	OpCodes *lval;
} ForinVar;

ForinVar *forinvar_new(unichar *varname, OpCodes *local, OpCodes *lval)
{
	ForinVar *r = malloc(sizeof(ForinVar));
	r->varname = varname;
	r->local = local;
	r->lval = lval;
	return r;
}

static OpCodes *make_forin(OpCodes *lval, OpCodes *expr, OpCodes *stat, const unichar *label)
{
	OpCodes *init = codes_join(expr, code_key());
	OpCodes *cond = codes_join3(lval, code_next(),
								   code_jfalse(stat->code_len + 2));
	OpCodes *stat_jmp = code_jmp(-(cond->code_len + stat->code_len));
	code_reserved_replace(stat, 1, 0, label, 2);
	return codes_join3(codes_join(init, cond), 
						  codes_join(stat, stat_jmp), code_pop(2));
}

typedef struct CaseExprStat {
	OpCodes *expr;
	OpCodes *stat;
	int isdefault;
} CaseExprStat;

CaseExprStat *exprstat_new(OpCodes *expr, OpCodes *stat, int isdef)
{
	CaseExprStat *r = malloc(sizeof(CaseExprStat));
	r->expr = expr;
	r->stat = stat;
	r->isdefault = isdef;
	return r;
}

typedef struct CaseList {
	CaseExprStat *es;
	int off;
	struct CaseList *tail;
	struct CaseList *next;
} CaseList;

static CaseList *caselist_new(CaseExprStat *es)
{
	CaseList *a = malloc(sizeof(CaseList));
	memset(a, 0, sizeof(CaseList));
	a->es = es;
	a->tail = a;
	return a;
}

static CaseList *caselist_insert(CaseList *a, CaseExprStat *es)
{
	CaseList *b = malloc(sizeof(CaseList));
	memset(b, 0, sizeof(CaseList));
	b->es = es;
	a->tail->next = b;
	a->tail = b;
	return a;
}

static OpCodes *opassign(OpCodes *lval, OpCodes *oprand, OpCodes *op)
{
	OpCodes *ret;
	if (((OpCodes *)lval)->lvalue_flag == 1) {
		ret = codes_join3(lval, 
							 codes_join3(code_push_top(), oprand, op),
							 code_assign(1));
	} else {
		ret = codes_join3(lval,
							 codes_join4(code_push_top2(), code_subscript(1), oprand, op),
							 code_assign(2));
	}
	return ret;
}



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 368 "parser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  98
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2111

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  89
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  41
/* YYNRULES -- Number of rules.  */
#define YYNRULES  164
/* YYNRULES -- Number of states.  */
#define YYNSTATES  332

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   319

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    73,     2,     2,     2,    72,    55,     2,
      82,    87,    70,    68,    35,    69,    80,    71,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    50,    84,
      61,    37,    60,    49,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    81,     2,    88,    54,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    85,    53,    86,    74,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      36,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    51,    52,    56,    57,    58,    59,    62,    63,
      64,    65,    66,    67,    75,    76,    77,    78,    79,    83
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     6,     9,    11,    13,    16,    18,
      20,    24,    27,    29,    31,    35,    39,    43,    46,    50,
      54,    56,    58,    60,    64,    66,    72,    75,    77,    79,
      81,    83,    84,    86,    87,    90,    92,    95,   101,   109,
     118,   120,   123,   128,   132,   140,   145,   155,   157,   161,
     163,   167,   171,   177,   185,   195,   204,   205,   208,   210,
     212,   215,   219,   220,   222,   223,   225,   232,   240,   246,
     253,   254,   256,   258,   262,   266,   269,   271,   273,   275,
     279,   284,   288,   291,   294,   297,   300,   303,   307,   311,
     315,   319,   323,   326,   329,   332,   335,   339,   343,   347,
     351,   355,   359,   363,   367,   371,   375,   379,   383,   387,
     391,   395,   399,   403,   407,   411,   415,   419,   423,   427,
     431,   435,   439,   443,   447,   451,   453,   456,   459,   464,
     467,   473,   479,   487,   493,   499,   504,   511,   519,   526,
     531,   533,   535,   537,   542,   546,   547,   549,   551,   555,
     557,   559,   561,   563,   565,   567,   569,   571,   575,   576,
     578,   582,   586,   590,   594
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      90,     0,    -1,    -1,    91,    -1,    91,   120,    -1,   120,
      -1,    92,    -1,    91,    92,    -1,    96,    -1,    93,    -1,
       4,    50,    93,    -1,   120,    84,    -1,   108,    -1,   107,
      -1,    15,    97,    84,    -1,    11,    97,    84,    -1,    17,
     120,    84,    -1,    17,    84,    -1,    18,   105,    84,    -1,
      24,   120,    84,    -1,   104,    -1,   100,    -1,    84,    -1,
      85,    91,    86,    -1,    94,    -1,    95,    82,   117,    87,
     119,    -1,    16,     4,    -1,   109,    -1,   114,    -1,   115,
      -1,   101,    -1,    -1,     4,    -1,    -1,     4,    50,    -1,
      92,    -1,    85,    86,    -1,    25,    82,   120,    87,    99,
      -1,    98,    12,    82,   120,    87,    85,    86,    -1,    98,
      12,    82,   120,    87,    85,   102,    86,    -1,   103,    -1,
     102,   103,    -1,    13,   120,    50,    91,    -1,    14,    50,
      91,    -1,    21,   119,    22,    82,     4,    87,   119,    -1,
      21,   119,    23,   119,    -1,    21,   119,    22,    82,     4,
      87,   119,    23,   119,    -1,   106,    -1,   105,    35,   106,
      -1,     4,    -1,     4,    37,   120,    -1,    20,   122,    84,
      -1,     5,    82,   120,    87,    99,    -1,     5,    82,   120,
      87,    99,     6,    99,    -1,    98,     7,    82,   111,   112,
      84,   113,    87,    99,    -1,    98,     7,    82,   110,     8,
     120,    87,    99,    -1,    -1,    18,     4,    -1,   122,    -1,
      84,    -1,   120,    84,    -1,    18,   105,    84,    -1,    -1,
     120,    -1,    -1,   120,    -1,    98,     9,    82,   120,    87,
      99,    -1,    98,    10,    99,     9,    82,   120,    87,    -1,
      16,    82,   117,    87,   119,    -1,    16,     4,    82,   117,
      87,   119,    -1,    -1,   118,    -1,     4,    -1,   118,    35,
       4,    -1,    85,    91,    86,    -1,    85,    86,    -1,   125,
      -1,   116,    -1,   122,    -1,   120,    35,   120,    -1,   120,
      81,   120,    88,    -1,   120,    80,     4,    -1,    69,   120,
      -1,    68,   120,    -1,    74,   120,    -1,    73,   120,    -1,
      75,   120,    -1,   120,    70,   120,    -1,   120,    71,   120,
      -1,   120,    72,   120,    -1,   120,    68,   120,    -1,   120,
      69,   120,    -1,   122,    78,    -1,   122,    77,    -1,    78,
     122,    -1,    77,   122,    -1,    82,   120,    87,    -1,   120,
      52,   120,    -1,   120,    51,   120,    -1,   120,    61,   120,
      -1,   120,    60,   120,    -1,   120,    64,   120,    -1,   120,
      63,   120,    -1,   120,    59,   120,    -1,   120,    58,   120,
      -1,   120,    57,   120,    -1,   120,    56,   120,    -1,   120,
      55,   120,    -1,   120,    53,   120,    -1,   120,    54,   120,
      -1,   120,    67,   120,    -1,   120,    66,   120,    -1,   120,
      65,   120,    -1,   122,    37,   120,    -1,   122,    48,   120,
      -1,   122,    47,   120,    -1,   122,    46,   120,    -1,   122,
      45,   120,    -1,   122,    44,   120,    -1,   122,    43,   120,
      -1,   122,    42,   120,    -1,   122,    41,   120,    -1,   122,
      40,   120,    -1,   122,    39,   120,    -1,   122,    38,   120,
      -1,   121,    -1,    19,   125,    -1,    19,   122,    -1,    19,
      82,   120,    87,    -1,    19,   116,    -1,    19,   125,    82,
     123,    87,    -1,    19,   122,    82,   123,    87,    -1,    19,
      82,   120,    87,    82,   123,    87,    -1,    19,   116,    82,
     123,    87,    -1,   120,    49,   120,    50,   120,    -1,    33,
      82,   120,    87,    -1,   120,    80,     4,    82,   123,    87,
      -1,   120,    81,   120,    88,    82,   123,    87,    -1,    82,
     120,    87,    82,   123,    87,    -1,   122,    82,   123,    87,
      -1,     4,    -1,    30,    -1,    29,    -1,   122,    81,   120,
      88,    -1,   122,    80,     4,    -1,    -1,   124,    -1,   120,
      -1,   124,    35,   120,    -1,     3,    -1,    26,    -1,    27,
      -1,    28,    -1,    31,    -1,    32,    -1,   126,    -1,   129,
      -1,    85,   127,    86,    -1,    -1,   128,    -1,   127,    35,
     128,    -1,     4,    50,   120,    -1,     3,    50,   120,    -1,
      81,   124,    88,    -1,    81,    88,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   188,   188,   189,   192,   195,   200,   201,   206,   207,
     208,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,   224,   225,   228,   239,   248,   249,   250,
     251,   254,   255,   258,   259,   265,   266,   270,   276,   277,
     318,   319,   323,   324,   328,   335,   342,   352,   353,   357,
     366,   378,   386,   390,   399,   410,   427,   428,   431,   439,
     440,   441,   444,   445,   448,   449,   453,   463,   473,   477,
     483,   484,   497,   498,   501,   502,   506,   507,   508,   512,
     513,   514,   515,   516,   517,   518,   519,   520,   521,   522,
     523,   524,   525,   529,   533,   537,   541,   542,   546,   550,
     551,   552,   553,   554,   555,   556,   557,   558,   559,   560,
     561,   562,   563,   564,   565,   566,   567,   568,   569,   570,
     571,   572,   573,   574,   575,   576,   578,   579,   582,   583,
     584,   589,   597,   602,   607,   611,   615,   621,   627,   632,
     652,   657,   658,   659,   664,   671,   672,   676,   677,   684,
     685,   686,   687,   688,   689,   690,   691,   695,   699,   700,
     701,   709,   710,   714,   715
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "STRING", "IDENTIFIER", "IF", "ELSE",
  "FOR", "IN", "WHILE", "DO", "CONTINUE", "SWITCH", "CASE", "DEFAULT",
  "BREAK", "FUNC", "RETURN", "LOCAL", "NEW", "DELETE", "TRY", "CATCH",
  "FINALLY", "THROW", "WITH", "UNDEF", "_TRUE", "_FALSE", "_THIS",
  "ARGUMENTS", "FNUMBER", "REGEXP", "__DEBUG", "MIN_PRI", "','",
  "ARGCOMMA", "'='", "DIVAS", "BXORAS", "BORAS", "BANDAS", "URSHFAS",
  "RSHFAS", "LSHFAS", "MODAS", "MULAS", "MNSAS", "ADDAS", "'?'", "':'",
  "OR", "AND", "'|'", "'^'", "'&'", "NNEQ", "EEQU", "NEQ", "EQU", "'>'",
  "'<'", "INSTANCEOF", "GEQ", "LEQ", "URSHF", "RSHF", "LSHF", "'+'", "'-'",
  "'*'", "'/'", "'%'", "'!'", "'~'", "VOID", "TYPEOF", "DEC", "INC", "NEG",
  "'.'", "'['", "'('", "MAX_PRI", "';'", "'{'", "'}'", "')'", "']'",
  "$accept", "file", "statements", "statement", "comonstatement",
  "func_statement", "func_prefix", "iterstatement", "identifier_opt",
  "label_opt", "statement_or_empty", "with_statement", "switch_statement",
  "cases", "case", "try_statement", "vardecs", "vardec",
  "delete_statement", "if_statement", "for_statement", "for_var",
  "for_init", "for_cond", "expr_opt", "while_statement", "do_statement",
  "func_expr", "args_opt", "args", "func_statement_block", "expr",
  "fcall_exprs", "lvalue", "exprlist_opt", "exprlist", "value", "object",
  "items", "item", "array", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,    44,   290,    61,   291,   292,
     293,   294,   295,   296,   297,   298,   299,   300,   301,    63,
      58,   302,   303,   124,    94,    38,   304,   305,   306,   307,
      62,    60,   308,   309,   310,   311,   312,   313,    43,    45,
      42,    47,    37,    33,   126,   314,   315,   316,   317,   318,
      46,    91,    40,   319,    59,   123,   125,    41,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    89,    90,    90,    90,    90,    91,    91,    92,    92,
      92,    93,    93,    93,    93,    93,    93,    93,    93,    93,
      93,    93,    93,    93,    93,    94,    95,    96,    96,    96,
      96,    97,    97,    98,    98,    99,    99,   100,   101,   101,
     102,   102,   103,   103,   104,   104,   104,   105,   105,   106,
     106,   107,   108,   108,   109,   109,   110,   110,   110,   111,
     111,   111,   112,   112,   113,   113,   114,   115,   116,   116,
     117,   117,   118,   118,   119,   119,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   121,   121,   121,   121,
     122,   122,   122,   122,   122,   123,   123,   124,   124,   125,
     125,   125,   125,   125,   125,   125,   125,   126,   127,   127,
     127,   128,   128,   129,   129
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     2,     1,     1,     2,     1,     1,
       3,     2,     1,     1,     3,     3,     3,     2,     3,     3,
       1,     1,     1,     3,     1,     5,     2,     1,     1,     1,
       1,     0,     1,     0,     2,     1,     2,     5,     7,     8,
       1,     2,     4,     3,     7,     4,     9,     1,     3,     1,
       3,     3,     5,     7,     9,     8,     0,     2,     1,     1,
       2,     3,     0,     1,     0,     1,     6,     7,     5,     6,
       0,     1,     1,     3,     3,     2,     1,     1,     1,     3,
       4,     3,     2,     2,     2,     2,     2,     3,     3,     3,
       3,     3,     2,     2,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     1,     2,     2,     4,     2,
       5,     5,     7,     5,     5,     4,     6,     7,     6,     4,
       1,     1,     1,     4,     3,     0,     1,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     0,     1,
       3,     3,     3,     3,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      33,   149,   140,     0,    31,    31,     0,     0,     0,     0,
       0,     0,     0,     0,   150,   151,   152,   142,   141,   153,
     154,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    22,    33,     0,    33,     6,     9,    24,     0,     8,
       0,    21,    30,    20,    13,    12,    27,    28,    29,    77,
       5,   125,    78,    76,   155,   156,    34,     0,    32,     0,
       0,    26,    70,   140,     0,    17,   158,     0,    49,     0,
      47,     0,   129,   127,   126,     0,    33,     0,     0,     0,
       0,    83,    82,    85,    84,    86,    95,    94,   164,   147,
       0,     0,   149,   140,    33,     0,     0,   159,     1,     7,
       4,    70,     0,     0,    33,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    11,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    93,    92,     0,     0,   145,    10,
       0,    15,    14,    70,    72,     0,    71,     0,     0,     0,
      16,     0,     0,    18,     0,   145,   145,   145,    51,    75,
      33,     0,     0,    19,     0,     0,     0,   163,    96,     0,
      34,    23,     0,   157,     0,    56,     0,    33,    35,     0,
       0,    79,     0,    98,    97,   108,   109,   107,   106,   105,
     104,   103,   100,    99,   102,   101,   112,   111,   110,    90,
      91,    87,    88,    89,    81,     0,   113,   124,   123,   122,
     121,   120,   119,   118,   117,   116,   115,   114,   144,     0,
       0,   146,    33,     0,     0,     0,     0,    50,    48,   128,
       0,     0,     0,    74,     0,    45,    33,   135,   148,   145,
     162,   161,   160,     0,     0,    59,     0,    62,     0,    78,
       0,    36,     0,     0,     0,   145,    80,   143,   139,    52,
       0,    68,    73,   161,   145,   133,   131,   130,     0,    37,
       0,    25,    49,     0,     0,     0,    63,    60,    33,     0,
       0,   134,     0,   145,    33,    69,     0,     0,   138,    61,
       0,    64,    66,     0,     0,   136,     0,    53,   132,    44,
      33,     0,    65,    67,     0,     0,    38,     0,    40,   137,
       0,    55,    33,     0,    33,    39,    41,    46,    54,    33,
      33,    33
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    33,    94,   188,    36,    37,    38,    39,    59,    40,
     189,    41,    42,   317,   318,    43,    69,    70,    44,    45,
      46,   256,   257,   285,   311,    47,    48,    49,   155,   156,
      77,    95,    51,    52,   230,   231,    53,    54,    96,    97,
      55
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -224
static const yytype_int16 yypact[] =
{
     202,  -224,   -32,   -53,    31,    31,     1,  1072,    40,    11,
      26,   -28,  1090,   -49,  -224,  -224,  -224,  -224,  -224,  -224,
    -224,   -20,  1090,  1090,  1090,  1090,  1090,    26,    26,   355,
    1090,  -224,   556,    60,   336,  -224,  -224,  -224,     7,  -224,
      63,  -224,  -224,  -224,  -224,  -224,  -224,  -224,  -224,  -224,
    1601,  -224,   572,  -224,  -224,  -224,   872,  1090,  -224,   -10,
      -2,  -224,    87,  -224,     6,  -224,    18,  1639,    69,   -23,
    -224,  1090,    15,    35,    39,    21,   673,    29,  1677,  1090,
    1090,    27,    27,    27,    27,    27,    38,    38,  -224,   187,
     -29,  1250,    50,    70,   757,  1601,   -22,  -224,  -224,  -224,
    1601,    87,    43,    45,   903,    47,  1090,  1090,  1090,  1090,
    1090,  1090,  1090,  1090,  1090,  1090,  1090,  1090,  1090,  1090,
    1090,  1090,  1090,  1090,  1090,  1090,  1090,  1090,  1090,   120,
    1090,  -224,  1090,  1090,  1090,  1090,  1090,  1090,  1090,  1090,
    1090,  1090,  1090,  1090,  -224,  -224,   151,  1090,  1090,  -224,
    1289,  -224,  -224,    87,  -224,    82,   138,    92,    50,   125,
    -224,  1090,    40,  -224,  1328,  1090,  1090,  1090,  -224,  -224,
     788,    96,   -28,  -224,  1367,  1406,  1090,  -224,    97,  1090,
     872,  -224,    18,  -224,    93,  1005,  1090,   640,  -224,   172,
    1090,   187,  1753,  1926,  1955,  1135,  1982,  2008,  2030,  2030,
    2030,  2030,   225,   225,   225,   225,    42,    42,    42,    -3,
      -3,    27,    27,    27,   100,  1174,   187,   187,   187,   187,
     187,   187,   187,   187,   187,   187,   187,   187,  -224,  1212,
      99,   148,   903,   101,   -28,   188,  1090,   187,  -224,   112,
     108,   109,   110,  -224,   194,  -224,   903,  -224,   187,  1090,
     187,  1862,  -224,   -28,   195,  -224,   193,  1090,  1715,  1140,
    1445,  -224,   126,  1484,  1090,  1090,   129,  -224,  -224,   209,
     -28,  -224,  -224,   187,  1090,  -224,  -224,  -224,   137,  -224,
     150,  -224,     8,   -18,  1090,   132,  1829,  -224,   903,  1090,
     140,  1896,   162,  1090,   903,  -224,   174,   -28,  -224,  -224,
    1523,  1090,  -224,  1562,    -6,  -224,   175,  -224,  -224,   240,
     903,   177,  1829,  -224,  1090,   215,  -224,    12,  -224,  -224,
     -28,  -224,   903,  1791,   986,  -224,  -224,  -224,  -224,   986,
     441,   472
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -224,  -224,     3,     0,   -52,  -224,  -224,  -224,   261,  -224,
    -223,  -224,  -224,  -224,   -48,  -224,    20,   116,  -224,  -224,
    -224,  -224,  -224,  -224,  -224,  -224,  -224,   263,   -90,  -224,
    -144,    24,  -224,    -8,   -81,   253,   276,  -224,  -224,   117,
    -224
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -159
static const yytype_int16 yytable[] =
{
      35,    73,    75,    34,   149,    61,   176,   314,   315,   269,
     157,   184,   162,   182,     1,    63,   -57,   162,    56,    86,
      87,   158,   159,   279,    50,   314,   315,    64,   245,    57,
      63,    67,    35,    79,    99,    58,    78,    14,    15,    16,
      17,    18,    19,    20,    68,   161,    81,    82,    83,    84,
      85,   171,   172,    89,    91,    17,    18,    76,   100,   177,
      98,   163,    80,   233,   183,   302,   299,   126,   127,   128,
     102,   307,   103,   104,   151,   105,    35,   129,   130,   170,
     316,   150,   152,    62,   240,   241,   242,   321,    62,   101,
     271,   154,    29,    71,    99,   164,    66,   165,   325,   328,
     179,   146,   147,   174,   175,   168,   161,   129,   130,   281,
     124,   125,   126,   127,   128,   146,   147,   166,   146,   147,
     180,   167,   129,   130,   214,   185,   295,   186,   149,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   309,   215,   228,   216,   217,   218,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   280,   234,
      99,   229,    89,   235,   153,   236,   327,   259,   244,   249,
     253,   262,   265,   176,   292,   237,   268,    35,   270,    89,
      89,    89,   272,   296,   274,   275,   276,   277,   278,   282,
     248,   284,    -2,   250,   251,     1,     2,     3,   289,   258,
     260,   293,   306,     4,   263,   294,   301,     5,     6,     7,
       8,     9,    10,    11,   297,   304,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,   107,   298,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   305,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     273,   308,   319,   320,   322,   324,    60,   129,   130,   326,
      22,    23,    72,    89,   283,    24,    25,    26,   238,    27,
      28,   286,    90,    29,    30,    74,    31,    32,   291,    89,
     121,   122,   123,   124,   125,   126,   127,   128,    89,   252,
       0,     0,     0,     0,     0,   129,   130,     0,   300,     0,
       0,     0,     0,   303,     0,     0,     0,    89,     0,     0,
       0,     0,     0,     0,    35,   312,     0,   330,     0,    35,
      99,    99,   331,     0,     0,     0,    -3,     0,   323,     1,
       2,     3,     0,     0,     0,     0,     0,     4,     0,     0,
       0,     5,     6,     7,     8,     9,    10,    11,     1,    63,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
       0,    64,     0,     0,     9,     0,     0,     0,     0,     0,
       0,    14,    15,    16,    17,    18,    19,    20,    21,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    22,    23,     0,     0,     0,    24,
      25,    26,     0,    27,    28,     0,     0,    29,    30,     0,
      31,    32,     0,    22,    23,     0,     0,     0,    24,    25,
      26,     0,    27,    28,     0,     0,    29,    30,     0,     0,
      66,     0,     0,    88,     1,     2,     3,     0,     0,     0,
       0,     0,     4,     0,   -43,   -43,     5,     6,     7,     8,
       9,    10,    11,     0,     0,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,     1,     2,     3,     0,     0,
       0,     0,     0,     4,     0,   -42,   -42,     5,     6,     7,
       8,     9,    10,    11,     0,     0,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,     0,     0,     0,    22,
      23,     0,     0,     0,    24,    25,    26,     0,    27,    28,
       0,     0,    29,    30,     0,    31,    32,   -43,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      22,    23,     0,     0,     0,    24,    25,    26,     0,    27,
      28,     0,     0,    29,    30,     0,    31,    32,   -42,    92,
      93,     3,     0,     0,     0,     0,     0,     4,     0,     0,
       0,     5,     6,     7,     8,     9,    10,    11,     0,     0,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
       0,  -158,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,     0,     0,     0,    22,    23,     0,     0,     0,    24,
      25,    26,     0,    27,    28,     0,     0,    29,    30,     0,
      31,    32,  -158,    92,    93,     3,     0,     0,     0,   144,
     145,     4,   146,   147,   148,     5,     6,     7,     8,     9,
      10,    11,     0,     0,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,     0,  -158,     1,     2,     3,     0,
       0,     0,     0,     0,     4,     0,     0,     0,     5,     6,
       7,     8,     9,    10,    11,     0,     0,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,     0,    22,    23,
       0,     0,     0,    24,    25,    26,     0,    27,    28,     0,
       0,    29,    30,     0,    31,    32,   261,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    22,    23,     0,     0,     0,    24,    25,    26,     0,
      27,    28,     0,     0,    29,    30,     0,    31,    32,   169,
       1,     2,     3,     0,     0,     0,     0,     0,     4,     0,
       0,     0,     5,     6,     7,     8,     9,    10,    11,     0,
       0,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,     1,     2,     3,     0,     0,     0,     0,     0,     4,
       0,     0,     0,     5,     6,     7,     8,     9,    10,    11,
       0,     0,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,     0,     0,     0,    22,    23,     0,     0,     0,
      24,    25,    26,     0,    27,    28,     0,     0,    29,    30,
       0,    31,    32,   181,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    22,    23,     0,     0,
       0,    24,    25,    26,     0,    27,    28,     0,     0,    29,
      30,     0,    31,    32,   243,     1,    63,     3,     0,     0,
       0,     0,     0,     4,     0,     0,     0,     5,     6,     7,
       8,     9,    10,    11,     0,     0,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,     1,     2,     3,     0,
       0,     0,     0,     0,     4,     0,     0,     0,     5,     6,
       7,     8,     9,    10,    11,     0,     0,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,     0,     0,     0,
      22,    23,     0,     0,     0,    24,    25,    26,     0,    27,
      28,     0,     0,    29,    30,     0,    31,    32,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    22,    23,     0,     0,     0,    24,    25,    26,     0,
      27,    28,     0,     0,    29,    30,     0,    31,   187,     1,
       2,     3,     0,     0,     0,     0,     0,     4,     0,     0,
       0,     5,     6,     7,     8,     9,    10,    11,     1,    63,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
       0,    64,     0,   254,     9,     0,     0,     0,     0,     0,
       0,    14,    15,    16,    17,    18,    19,    20,    21,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    22,    23,     0,     0,     0,    24,
      25,    26,     0,    27,    28,     0,     0,    29,    30,     0,
      31,    32,     0,    22,    23,     1,    63,     0,    24,    25,
      26,     0,    27,    28,     0,     0,    29,    30,    64,   255,
      66,     9,     0,     1,    63,     0,     0,     0,    14,    15,
      16,    17,    18,    19,    20,    21,    64,     0,     0,     9,
       0,     0,     0,     0,     0,     0,    14,    15,    16,    17,
      18,    19,    20,    21,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      22,    23,     0,     0,     0,    24,    25,    26,   -58,    27,
      28,     0,     0,    29,    30,     0,    65,    66,    22,    23,
       0,     0,     0,    24,    25,    26,     0,    27,    28,     0,
       0,    29,    30,     0,     0,    66,     0,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   111,
     112,   113,   114,   115,   116,   117,   118,     0,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,     0,   106,
       0,     0,     0,     0,     0,   129,   130,   144,   145,     0,
     146,   147,   148,   107,     0,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,     0,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   106,     0,     0,
       0,     0,     0,     0,   129,   130,     0,     0,     0,     0,
       0,   107,   266,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,     0,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   106,     0,     0,     0,     0,
       0,     0,   129,   130,     0,     0,     0,     0,     0,   107,
     267,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,     0,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,     0,   106,     0,     0,     0,     0,     0,
     129,   130,     0,     0,     0,     0,     0,   178,   107,     0,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,     0,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,     0,   106,     0,     0,     0,     0,     0,   129,
     130,     0,     0,     0,     0,     0,   232,   107,     0,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
       0,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,     0,   106,     0,     0,     0,     0,     0,   129,   130,
       0,     0,     0,     0,     0,   239,   107,     0,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,     0,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
       0,   106,     0,     0,     0,     0,     0,   129,   130,     0,
       0,     0,     0,     0,   246,   107,     0,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,     0,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,     0,
     106,     0,     0,     0,     0,     0,   129,   130,     0,     0,
       0,     0,     0,   247,   107,     0,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,     0,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,     0,   106,
       0,     0,     0,     0,     0,   129,   130,     0,     0,     0,
       0,     0,   288,   107,     0,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,     0,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,     0,   106,     0,
       0,     0,     0,     0,   129,   130,     0,     0,     0,     0,
       0,   290,   107,     0,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,     0,   119,   120,   121,   122,
     123,   124,   125,   126,   127,   128,     0,   106,     0,     0,
       0,     0,     0,   129,   130,     0,     0,     0,     0,     0,
     310,   107,     0,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,     0,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,     0,   106,     0,     0,     0,
       0,     0,   129,   130,     0,     0,     0,     0,     0,   313,
     107,     0,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,     0,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   106,     0,     0,     0,     0,     0,
       0,   129,   130,     0,     0,   131,     0,     0,   107,     0,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,     0,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   106,     0,     0,     0,     0,     0,     0,   129,
     130,     0,     0,   160,     0,     0,   107,     0,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,     0,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     106,     0,     0,     0,     0,     0,     0,   129,   130,     0,
       0,   173,     0,     0,   107,     0,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,     0,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   106,     0,
       0,     0,     0,     0,     0,   129,   130,     0,     0,   287,
       0,     0,   107,   264,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,     0,   119,   120,   121,   122,
     123,   124,   125,   126,   127,   128,   106,     0,     0,     0,
       0,     0,     0,   129,   130,     0,     0,     0,     0,     0,
     107,   329,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,     0,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   106,     0,     0,     0,     0,     0,
       0,   129,   130,     0,     0,     0,     0,     0,   107,     0,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,     0,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,     0,     0,     0,     0,     0,     0,     0,   129,
     130,   107,     0,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,     0,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,     0,     0,     0,     0,     0,
       0,     0,   129,   130,     0,     0,   131,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,     0,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,     0,
       0,     0,     0,     0,     0,     0,   129,   130,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,     0,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,     0,
       0,     0,     0,     0,     0,     0,   129,   130,   110,   111,
     112,   113,   114,   115,   116,   117,   118,     0,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,     0,     0,
       0,     0,     0,     0,     0,   129,   130,   112,   113,   114,
     115,   116,   117,   118,     0,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,     0,     0,     0,     0,     0,
       0,     0,   129,   130,   113,   114,   115,   116,   117,   118,
       0,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,     0,     0,     0,     0,     0,     0,     0,   129,   130,
     117,   118,     0,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,     0,     0,     0,     0,     0,     0,     0,
     129,   130
};

static const yytype_int16 yycheck[] =
{
       0,     9,    10,     0,    56,     4,    35,    13,    14,   232,
       4,   101,    35,    35,     3,     4,     8,    35,    50,    27,
      28,     3,     4,   246,     0,    13,    14,    16,   172,    82,
       4,     7,    32,    82,    34,     4,    12,    26,    27,    28,
      29,    30,    31,    32,     4,    37,    22,    23,    24,    25,
      26,    22,    23,    29,    30,    29,    30,    85,    34,    88,
       0,    84,    82,   153,    86,   288,    84,    70,    71,    72,
       7,   294,     9,    10,    84,    12,    76,    80,    81,    76,
      86,    57,    84,    82,   165,   166,   167,   310,    82,    82,
     234,     4,    81,    82,    94,    71,    85,    82,    86,   322,
      50,    80,    81,    79,    80,    84,    37,    80,    81,   253,
      68,    69,    70,    71,    72,    80,    81,    82,    80,    81,
      50,    82,    80,    81,     4,    82,   270,    82,   180,    82,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   297,   130,     4,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   249,    87,
     170,   147,   148,    35,    82,    50,   320,   185,    82,    82,
      87,     9,    82,    35,   265,   161,    87,   187,    87,   165,
     166,   167,     4,   274,    82,    87,    87,    87,     4,     4,
     176,     8,     0,   179,   180,     3,     4,     5,    82,   185,
     186,    82,   293,    11,   190,     6,    84,    15,    16,    17,
      18,    19,    20,    21,    87,    85,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    49,    87,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    87,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
     236,    87,    87,    23,    87,    50,     5,    80,    81,   317,
      68,    69,     9,   249,   254,    73,    74,    75,   162,    77,
      78,   257,    29,    81,    82,     9,    84,    85,   264,   265,
      65,    66,    67,    68,    69,    70,    71,    72,   274,   182,
      -1,    -1,    -1,    -1,    -1,    80,    81,    -1,   284,    -1,
      -1,    -1,    -1,   289,    -1,    -1,    -1,   293,    -1,    -1,
      -1,    -1,    -1,    -1,   324,   301,    -1,   324,    -1,   329,
     330,   331,   329,    -1,    -1,    -1,     0,    -1,   314,     3,
       4,     5,    -1,    -1,    -1,    -1,    -1,    11,    -1,    -1,
      -1,    15,    16,    17,    18,    19,    20,    21,     3,     4,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    16,    -1,    -1,    19,    -1,    -1,    -1,    -1,    -1,
      -1,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    68,    69,    -1,    -1,    -1,    73,
      74,    75,    -1,    77,    78,    -1,    -1,    81,    82,    -1,
      84,    85,    -1,    68,    69,    -1,    -1,    -1,    73,    74,
      75,    -1,    77,    78,    -1,    -1,    81,    82,    -1,    -1,
      85,    -1,    -1,    88,     3,     4,     5,    -1,    -1,    -1,
      -1,    -1,    11,    -1,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    -1,    -1,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,     3,     4,     5,    -1,    -1,
      -1,    -1,    -1,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    -1,    -1,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    -1,    -1,    68,
      69,    -1,    -1,    -1,    73,    74,    75,    -1,    77,    78,
      -1,    -1,    81,    82,    -1,    84,    85,    86,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      68,    69,    -1,    -1,    -1,    73,    74,    75,    -1,    77,
      78,    -1,    -1,    81,    82,    -1,    84,    85,    86,     3,
       4,     5,    -1,    -1,    -1,    -1,    -1,    11,    -1,    -1,
      -1,    15,    16,    17,    18,    19,    20,    21,    -1,    -1,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    35,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    -1,    -1,    -1,    68,    69,    -1,    -1,    -1,    73,
      74,    75,    -1,    77,    78,    -1,    -1,    81,    82,    -1,
      84,    85,    86,     3,     4,     5,    -1,    -1,    -1,    77,
      78,    11,    80,    81,    82,    15,    16,    17,    18,    19,
      20,    21,    -1,    -1,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    -1,    35,     3,     4,     5,    -1,
      -1,    -1,    -1,    -1,    11,    -1,    -1,    -1,    15,    16,
      17,    18,    19,    20,    21,    -1,    -1,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    -1,    68,    69,
      -1,    -1,    -1,    73,    74,    75,    -1,    77,    78,    -1,
      -1,    81,    82,    -1,    84,    85,    86,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    68,    69,    -1,    -1,    -1,    73,    74,    75,    -1,
      77,    78,    -1,    -1,    81,    82,    -1,    84,    85,    86,
       3,     4,     5,    -1,    -1,    -1,    -1,    -1,    11,    -1,
      -1,    -1,    15,    16,    17,    18,    19,    20,    21,    -1,
      -1,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,     3,     4,     5,    -1,    -1,    -1,    -1,    -1,    11,
      -1,    -1,    -1,    15,    16,    17,    18,    19,    20,    21,
      -1,    -1,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    -1,    -1,    -1,    68,    69,    -1,    -1,    -1,
      73,    74,    75,    -1,    77,    78,    -1,    -1,    81,    82,
      -1,    84,    85,    86,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    68,    69,    -1,    -1,
      -1,    73,    74,    75,    -1,    77,    78,    -1,    -1,    81,
      82,    -1,    84,    85,    86,     3,     4,     5,    -1,    -1,
      -1,    -1,    -1,    11,    -1,    -1,    -1,    15,    16,    17,
      18,    19,    20,    21,    -1,    -1,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,     3,     4,     5,    -1,
      -1,    -1,    -1,    -1,    11,    -1,    -1,    -1,    15,    16,
      17,    18,    19,    20,    21,    -1,    -1,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    -1,    -1,    -1,
      68,    69,    -1,    -1,    -1,    73,    74,    75,    -1,    77,
      78,    -1,    -1,    81,    82,    -1,    84,    85,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    68,    69,    -1,    -1,    -1,    73,    74,    75,    -1,
      77,    78,    -1,    -1,    81,    82,    -1,    84,    85,     3,
       4,     5,    -1,    -1,    -1,    -1,    -1,    11,    -1,    -1,
      -1,    15,    16,    17,    18,    19,    20,    21,     3,     4,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    16,    -1,    18,    19,    -1,    -1,    -1,    -1,    -1,
      -1,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    68,    69,    -1,    -1,    -1,    73,
      74,    75,    -1,    77,    78,    -1,    -1,    81,    82,    -1,
      84,    85,    -1,    68,    69,     3,     4,    -1,    73,    74,
      75,    -1,    77,    78,    -1,    -1,    81,    82,    16,    84,
      85,    19,    -1,     3,     4,    -1,    -1,    -1,    26,    27,
      28,    29,    30,    31,    32,    33,    16,    -1,    -1,    19,
      -1,    -1,    -1,    -1,    -1,    -1,    26,    27,    28,    29,
      30,    31,    32,    33,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      68,    69,    -1,    -1,    -1,    73,    74,    75,     8,    77,
      78,    -1,    -1,    81,    82,    -1,    84,    85,    68,    69,
      -1,    -1,    -1,    73,    74,    75,    -1,    77,    78,    -1,
      -1,    81,    82,    -1,    -1,    85,    -1,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    -1,    35,
      -1,    -1,    -1,    -1,    -1,    80,    81,    77,    78,    -1,
      80,    81,    82,    49,    -1,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    -1,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    35,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    81,    -1,    -1,    -1,    -1,
      -1,    49,    88,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    -1,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    35,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    81,    -1,    -1,    -1,    -1,    -1,    49,
      88,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    -1,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    -1,    35,    -1,    -1,    -1,    -1,    -1,
      80,    81,    -1,    -1,    -1,    -1,    -1,    87,    49,    -1,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    -1,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    -1,    35,    -1,    -1,    -1,    -1,    -1,    80,
      81,    -1,    -1,    -1,    -1,    -1,    87,    49,    -1,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      -1,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    -1,    35,    -1,    -1,    -1,    -1,    -1,    80,    81,
      -1,    -1,    -1,    -1,    -1,    87,    49,    -1,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    35,    -1,    -1,    -1,    -1,    -1,    80,    81,    -1,
      -1,    -1,    -1,    -1,    87,    49,    -1,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    -1,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    -1,
      35,    -1,    -1,    -1,    -1,    -1,    80,    81,    -1,    -1,
      -1,    -1,    -1,    87,    49,    -1,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    -1,    35,
      -1,    -1,    -1,    -1,    -1,    80,    81,    -1,    -1,    -1,
      -1,    -1,    87,    49,    -1,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    -1,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    -1,    35,    -1,
      -1,    -1,    -1,    -1,    80,    81,    -1,    -1,    -1,    -1,
      -1,    87,    49,    -1,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    -1,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    35,    -1,    -1,
      -1,    -1,    -1,    80,    81,    -1,    -1,    -1,    -1,    -1,
      87,    49,    -1,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    -1,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    35,    -1,    -1,    -1,
      -1,    -1,    80,    81,    -1,    -1,    -1,    -1,    -1,    87,
      49,    -1,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    -1,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    35,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    81,    -1,    -1,    84,    -1,    -1,    49,    -1,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    -1,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    35,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      81,    -1,    -1,    84,    -1,    -1,    49,    -1,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      35,    -1,    -1,    -1,    -1,    -1,    -1,    80,    81,    -1,
      -1,    84,    -1,    -1,    49,    -1,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    35,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    81,    -1,    -1,    84,
      -1,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    -1,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    35,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    81,    -1,    -1,    -1,    -1,    -1,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    -1,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    35,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    81,    -1,    -1,    -1,    -1,    -1,    49,    -1,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    -1,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      81,    49,    -1,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    -1,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    81,    -1,    -1,    84,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    -1,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    81,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    -1,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    81,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    81,    55,    56,    57,
      58,    59,    60,    61,    -1,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    81,    56,    57,    58,    59,    60,    61,
      -1,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    81,
      60,    61,    -1,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    81
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,    11,    15,    16,    17,    18,    19,
      20,    21,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    68,    69,    73,    74,    75,    77,    78,    81,
      82,    84,    85,    90,    91,    92,    93,    94,    95,    96,
      98,   100,   101,   104,   107,   108,   109,   114,   115,   116,
     120,   121,   122,   125,   126,   129,    50,    82,     4,    97,
      97,     4,    82,     4,    16,    84,    85,   120,     4,   105,
     106,    82,   116,   122,   125,   122,    85,   119,   120,    82,
      82,   120,   120,   120,   120,   120,   122,   122,    88,   120,
     124,   120,     3,     4,    91,   120,   127,   128,     0,    92,
     120,    82,     7,     9,    10,    12,    35,    49,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    80,
      81,    84,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    77,    78,    80,    81,    82,    93,
     120,    84,    84,    82,     4,   117,   118,     4,     3,     4,
      84,    37,    35,    84,   120,    82,    82,    82,    84,    86,
      91,    22,    23,    84,   120,   120,    35,    88,    87,    50,
      50,    86,    35,    86,   117,    82,    82,    85,    92,    99,
      82,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,     4,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,     4,   120,
     123,   124,    87,   117,    87,    35,    50,   120,   106,    87,
     123,   123,   123,    86,    82,   119,    87,    87,   120,    82,
     120,   120,   128,    87,    18,    84,   110,   111,   120,   122,
     120,    86,     9,   120,    50,    82,    88,    88,    87,    99,
      87,   119,     4,   120,    82,    87,    87,    87,     4,    99,
     123,   119,     4,   105,     8,   112,   120,    84,    87,    82,
      87,   120,   123,    82,     6,   119,   123,    87,    87,    84,
     120,    84,    99,   120,    85,    87,   123,    99,    87,   119,
      87,   113,   120,    87,    13,    14,    86,   102,   103,    87,
      23,    99,    87,   120,    50,    86,   103,   119,    99,    50,
      91,    91
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (&yylloc, pstate, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc, pstate)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location, pstate); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, PSTATE *pstate)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, pstate)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    PSTATE *pstate;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (pstate);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, PSTATE *pstate)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, pstate)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    PSTATE *pstate;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, pstate);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, PSTATE *pstate)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, pstate)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    PSTATE *pstate;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , pstate);
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, pstate); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, PSTATE *pstate)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, pstate)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    PSTATE *pstate;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (pstate);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (PSTATE *pstate);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (PSTATE *pstate)
#else
int
yyparse (pstate)
    PSTATE *pstate;
#endif
#endif
{
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;
#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 188 "parser.y"
    { pstate->opcodes = code_nop(); ;}
    break;

  case 3:
#line 189 "parser.y"
    {
		pstate->opcodes = (yyvsp[(1) - (1)]);
	;}
    break;

  case 4:
#line 192 "parser.y"
    {
		pstate->opcodes = codes_join3((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)]), code_ret(1));
	;}
    break;

  case 5:
#line 195 "parser.y"
    {	/* for json */
		pstate->opcodes = codes_join((yyvsp[(1) - (1)]), code_ret(1));
	;}
    break;

  case 6:
#line 200 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 7:
#line 201 "parser.y"
    { (yyval) = codes_join((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); ;}
    break;

  case 8:
#line 206 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 9:
#line 207 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 10:
#line 208 "parser.y"
    { (yyval) = (yyvsp[(3) - (3)]); ;}
    break;

  case 11:
#line 212 "parser.y"
    { (yyval) = codes_join((yyvsp[(1) - (2)]), code_pop(1)); ;}
    break;

  case 12:
#line 213 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 13:
#line 214 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 14:
#line 215 "parser.y"
    { (yyval) = code_reserved(RES_BREAK, (yyvsp[(2) - (3)])); ;}
    break;

  case 15:
#line 216 "parser.y"
    { (yyval) = code_reserved(RES_CONTINUE, (yyvsp[(2) - (3)])); ;}
    break;

  case 16:
#line 217 "parser.y"
    { (yyval) = codes_join((yyvsp[(2) - (3)]), code_ret(1)); ;}
    break;

  case 17:
#line 218 "parser.y"
    { (yyval) = code_ret(0); ;}
    break;

  case 18:
#line 219 "parser.y"
    { (yyval) = (yyvsp[(2) - (3)]); ;}
    break;

  case 19:
#line 220 "parser.y"
    { (yyval) = codes_join((yyvsp[(2) - (3)]), code_throw()); ;}
    break;

  case 20:
#line 221 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 21:
#line 222 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 22:
#line 223 "parser.y"
    { (yyval) = code_nop(); ;}
    break;

  case 23:
#line 224 "parser.y"
    { (yyval) = (yyvsp[(2) - (3)]); ;}
    break;

  case 24:
#line 225 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 25:
#line 228 "parser.y"
    {
		OpCodes *ret = codes_join4(code_push_index((yyvsp[(1) - (5)])),
									  code_push_func(func_make_static((yyvsp[(3) - (5)]), scope_get_varlist(), (yyvsp[(5) - (5)]))),
									  code_assign(1), code_pop(1));
		if (pstate->eval_flag) ret = codes_join(code_local((yyvsp[(1) - (5)])), ret);
		scope_pop();
		(yyval) = ret;
	;}
    break;

  case 26:
#line 239 "parser.y"
    {
		if (!pstate->eval_flag) {
			scope_add_var((yyvsp[(2) - (2)]));
		}
		(yyval) = (yyvsp[(2) - (2)]);
	;}
    break;

  case 27:
#line 248 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 28:
#line 249 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 29:
#line 250 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 30:
#line 251 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 31:
#line 254 "parser.y"
    { (yyval) = NULL; ;}
    break;

  case 32:
#line 255 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 33:
#line 258 "parser.y"
    { (yyval) = NULL; ;}
    break;

  case 34:
#line 259 "parser.y"
    {
		(yyval) = (yyvsp[(1) - (2)]);
	;}
    break;

  case 35:
#line 265 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 36:
#line 266 "parser.y"
    { (yyval) = code_nop(); ;}
    break;

  case 37:
#line 270 "parser.y"
    { 
		(yyval) = codes_join4((yyvsp[(3) - (5)]), code_with(((OpCodes *)(yyvsp[(5) - (5)]))->code_len + 1), (yyvsp[(5) - (5)]), code_ewith());
	;}
    break;

  case 38:
#line 276 "parser.y"
    { (yyval) = codes_join((yyvsp[(4) - (7)]), code_pop(1)); ;}
    break;

  case 39:
#line 277 "parser.y"
    {
		CaseList *cl = (yyvsp[(7) - (8)]);
		OpCodes *allstats = codes_new(3);
		CaseList *cldefault = NULL;
		CaseList *head = NULL;
		
		while (cl) {
			cl->off = allstats->code_len;
			allstats = codes_join(allstats, cl->es->stat);

			CaseList *t = cl;
			cl = cl->next;
			
			if (t->es->isdefault) {
				if (cldefault) yyerror(&(yylsp[(8) - (8)]), pstate, "More then one switch default\n");
				cldefault = t;
			} else {
				t->next = head;
				head = t;
			}
		}
		code_reserved_replace(allstats, 0, 1, (yyvsp[(1) - (8)]), 1);
		
		OpCodes *ophead = code_jmp(allstats->code_len + 1);
		if (cldefault) {
			ophead = codes_join(code_jmp(ophead->code_len + cldefault->off + 1), ophead);
			free(cldefault);
		}
		while (head) {
			OpCodes *e = codes_join4(code_push_top(), head->es->expr, 
										code_equal(), code_jtrue(ophead->code_len + head->off + 1));
			ophead = codes_join(e, ophead);
			CaseList *t = head;
			head = head->next;
			free(t);
		}
		(yyval) = codes_join4(codes_join((yyvsp[(4) - (8)]), code_unref()), ophead, allstats, code_pop(1));
	;}
    break;

  case 40:
#line 318 "parser.y"
    { (yyval) = caselist_new((yyvsp[(1) - (1)])); ;}
    break;

  case 41:
#line 319 "parser.y"
    { (yyval) = caselist_insert((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); ;}
    break;

  case 42:
#line 323 "parser.y"
    { (yyval) = exprstat_new((yyvsp[(2) - (4)]), (yyvsp[(4) - (4)]), 0); ;}
    break;

  case 43:
#line 324 "parser.y"
    { (yyval) = exprstat_new(NULL, (yyvsp[(3) - (3)]), 1); ;}
    break;

  case 44:
#line 328 "parser.y"
    {
		OpCodes *catchblock = codes_join3(code_scatch((yyvsp[(5) - (7)])), (yyvsp[(7) - (7)]), code_ecatch());
		OpCodes *finallyblock = codes_join(code_sfinal(), code_efinal());
		OpCodes *tryblock = codes_join((yyvsp[(2) - (7)]), code_etry());
		(yyval) = codes_join4(code_stry(tryblock->code_len, catchblock->code_len, finallyblock->code_len),
							tryblock, catchblock, finallyblock);
	;}
    break;

  case 45:
#line 335 "parser.y"
    {
		OpCodes *catchblock = codes_join(code_scatch(NULL), code_ecatch());
		OpCodes *finallyblock = codes_join3(code_sfinal(), (yyvsp[(4) - (4)]), code_efinal());
		OpCodes *tryblock = codes_join((yyvsp[(2) - (4)]), code_etry());
		(yyval) = codes_join4(code_stry(tryblock->code_len, catchblock->code_len, finallyblock->code_len),
							tryblock, catchblock, finallyblock);
	;}
    break;

  case 46:
#line 343 "parser.y"
    {
		OpCodes *catchblock = codes_join3(code_scatch((yyvsp[(5) - (9)])), (yyvsp[(7) - (9)]), code_ecatch());
		OpCodes *finallyblock = codes_join3(code_sfinal(), (yyvsp[(9) - (9)]), code_efinal());
		OpCodes *tryblock = codes_join((yyvsp[(2) - (9)]), code_etry());
		(yyval) = codes_join4(code_stry(tryblock->code_len, catchblock->code_len, finallyblock->code_len),
							tryblock, catchblock, finallyblock);
	;}
    break;

  case 47:
#line 352 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 48:
#line 353 "parser.y"
    { (yyval) = codes_join((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 49:
#line 357 "parser.y"
    {
		OpCodes *ret = codes_join4(code_push_index((yyvsp[(1) - (1)])),
							code_push_undef(),
							code_assign(1),
							code_pop(1));
		if (!pstate->eval_flag)	scope_add_var((yyvsp[(1) - (1)]));
		else ret = codes_join(code_local((yyvsp[(1) - (1)])), ret);
		(yyval) = ret;
	;}
    break;

  case 50:
#line 366 "parser.y"
    {
		OpCodes *ret = codes_join4(code_push_index((yyvsp[(1) - (3)])),
							(yyvsp[(3) - (3)]),
							code_assign(1),
							code_pop(1));
		if (!pstate->eval_flag) scope_add_var((yyvsp[(1) - (3)]));
		else ret = codes_join(code_local((yyvsp[(1) - (3)])), ret);
		(yyval) = ret;
	;}
    break;

  case 51:
#line 378 "parser.y"
    {
		if (((OpCodes *)(yyvsp[(2) - (3)]))->lvalue_flag == 2) {
			(yyval) = codes_join((yyvsp[(2) - (3)]), code_delete(2));
		} else (yyval) = code_delete(1);
	;}
    break;

  case 52:
#line 386 "parser.y"
    {
		int offset = ((OpCodes *)(yyvsp[(5) - (5)]))->code_len;
		(yyval) = codes_join3((yyvsp[(3) - (5)]), code_jfalse(offset + 1), (yyvsp[(5) - (5)]));
	;}
    break;

  case 53:
#line 390 "parser.y"
    {
		int len_block2 = ((OpCodes *)(yyvsp[(7) - (7)]))->code_len;
		OpCodes *block1 = codes_join((yyvsp[(5) - (7)]), code_jmp(len_block2 + 1));
		OpCodes *condi = codes_join((yyvsp[(3) - (7)]), code_jfalse(block1->code_len + 1));
		(yyval) = codes_join3(condi, block1, (yyvsp[(7) - (7)]));
	;}
    break;

  case 54:
#line 399 "parser.y"
    {
		OpCodes *init = (yyvsp[(4) - (9)]);
		OpCodes *cond = (yyvsp[(5) - (9)]);
		OpCodes *step = ((yyvsp[(7) - (9)]) ? codes_join((yyvsp[(7) - (9)]), code_pop(1)) : code_nop());
		OpCodes *stat = (yyvsp[(9) - (9)]);
		OpCodes *cont_jmp = code_jfalse(step->code_len + stat->code_len + 2);
		OpCodes *step_jmp = code_jmp(-(cond->code_len + step->code_len + stat->code_len + 1));
		code_reserved_replace(stat, step->code_len + 1, 0, (yyvsp[(1) - (9)]), 0);
		(yyval) = codes_join(codes_join3(init, cond, cont_jmp),
						   codes_join3(stat, step, step_jmp));
	;}
    break;

  case 55:
#line 410 "parser.y"
    {
		ForinVar *fv = (yyvsp[(4) - (8)]);
		OpCodes *lval;
		if (fv->varname) lval = code_push_index(fv->varname);
		else lval = fv->lval;
		
		OpCodes *ret = make_forin(lval, (yyvsp[(6) - (8)]), (yyvsp[(8) - (8)]), (yyvsp[(1) - (8)]));
		if (fv->varname && fv->local) {
			if (!pstate->eval_flag) {
				scope_add_var(fv->varname);
				codes_free(fv->local);
			} else ret = codes_join(fv->local, ret);
		}
		(yyval) = ret;
	;}
    break;

  case 57:
#line 428 "parser.y"
    {
		(yyval) = forinvar_new((yyvsp[(2) - (2)]), code_local((yyvsp[(2) - (2)])), NULL);
	;}
    break;

  case 58:
#line 431 "parser.y"
    {
		if (((OpCodes *)(yyvsp[(1) - (1)]))->lvalue_flag == 2) 
			(yyval) = forinvar_new(NULL, NULL, codes_join((yyvsp[(1) - (1)]), code_subscript(0)));
		else (yyval) = forinvar_new(NULL, NULL, (yyvsp[(1) - (1)]));
	;}
    break;

  case 59:
#line 439 "parser.y"
    { (yyval) = code_nop(); ;}
    break;

  case 60:
#line 440 "parser.y"
    { (yyval) = codes_join((yyvsp[(1) - (2)]), code_pop(1)); ;}
    break;

  case 61:
#line 441 "parser.y"
    { (yyval) = (yyvsp[(2) - (3)]); ;}
    break;

  case 62:
#line 444 "parser.y"
    { (yyval) = code_push_bool(1); ;}
    break;

  case 63:
#line 445 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 64:
#line 448 "parser.y"
    { (yyval) = NULL; ;}
    break;

  case 65:
#line 449 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 66:
#line 453 "parser.y"
    {
		OpCodes *cond = (yyvsp[(4) - (6)]);
		OpCodes *stat = (yyvsp[(6) - (6)]);
		code_reserved_replace(stat, 1, 0, (yyvsp[(1) - (6)]), 0);
		(yyval) = codes_join4(cond, code_jfalse(stat->code_len + 2), stat,
						   code_jmp(-(stat->code_len + cond->code_len + 1)));
	;}
    break;

  case 67:
#line 463 "parser.y"
    {
		OpCodes *stat = (yyvsp[(3) - (7)]);
		OpCodes *cond = (yyvsp[(6) - (7)]);
		code_reserved_replace(stat, cond->code_len + 1, 0, (yyvsp[(1) - (7)]), 0);
		(yyval) = codes_join3(stat, cond,
							code_jtrue(-(stat->code_len + cond->code_len)));
	;}
    break;

  case 68:
#line 473 "parser.y"
    {
		(yyval) = code_push_func(func_make_static((yyvsp[(3) - (5)]), scope_get_varlist(), (yyvsp[(5) - (5)])));
		scope_pop();
	;}
    break;

  case 69:
#line 477 "parser.y"
    {
		(yyval) = code_push_func(func_make_static((yyvsp[(4) - (6)]), scope_get_varlist(), (yyvsp[(6) - (6)])));
		scope_pop();
	;}
    break;

  case 70:
#line 483 "parser.y"
    { scope_push(); (yyval) = strs_new(); ;}
    break;

  case 71:
#line 484 "parser.y"
    {
		scope_push();
		ArgList *a = (yyvsp[(1) - (1)]);
		strs *s = strs_new();
		while (a) {
			strs_push(s, a->argname);
			a = a->next;
		}
		(yyval) = s;
	;}
    break;

  case 72:
#line 497 "parser.y"
    { (yyval) = arglist_new((yyvsp[(1) - (1)])); ;}
    break;

  case 73:
#line 498 "parser.y"
    { (yyval) = arglist_insert((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 74:
#line 501 "parser.y"
    { (yyval) = (yyvsp[(2) - (3)]); ;}
    break;

  case 75:
#line 502 "parser.y"
    { (yyval) = code_nop(); ;}
    break;

  case 76:
#line 506 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 77:
#line 507 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 78:
#line 508 "parser.y"
    { 
		if (((OpCodes *)(yyvsp[(1) - (1)]))->lvalue_flag == 2) (yyval) = codes_join((yyvsp[(1) - (1)]), code_subscript(1)); 
		else (yyval) = (yyvsp[(1) - (1)]);
	;}
    break;

  case 79:
#line 512 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), code_pop(1), (yyvsp[(3) - (3)])); ;}
    break;

  case 80:
#line 513 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (4)]), (yyvsp[(3) - (4)]), code_subscript(1)); ;}
    break;

  case 81:
#line 514 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), code_push_string((yyvsp[(3) - (3)])), code_subscript(1)); ;}
    break;

  case 82:
#line 515 "parser.y"
    { (yyval) = codes_join((yyvsp[(2) - (2)]), code_neg()); ;}
    break;

  case 83:
#line 516 "parser.y"
    { (yyval) = codes_join((yyvsp[(2) - (2)]), code_pos()); ;}
    break;

  case 84:
#line 517 "parser.y"
    { (yyval) = codes_join((yyvsp[(2) - (2)]), code_bnot()); ;}
    break;

  case 85:
#line 518 "parser.y"
    { (yyval) = codes_join((yyvsp[(2) - (2)]), code_not()); ;}
    break;

  case 86:
#line 519 "parser.y"
    { (yyval) = codes_join3((yyvsp[(2) - (2)]), code_pop(1), code_push_undef()); ;}
    break;

  case 87:
#line 520 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_mul()); ;}
    break;

  case 88:
#line 521 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_div()); ;}
    break;

  case 89:
#line 522 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_mod()); ;}
    break;

  case 90:
#line 523 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_add()); ;}
    break;

  case 91:
#line 524 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_sub()); ;}
    break;

  case 92:
#line 525 "parser.y"
    {
 		if (((OpCodes *)(yyvsp[(1) - (2)]))->lvalue_flag == 2) (yyval) = codes_join3((yyvsp[(1) - (2)]), code_subscript(0), code_inc(1));
 		else (yyval) = codes_join((yyvsp[(1) - (2)]), code_inc(1));
 	;}
    break;

  case 93:
#line 529 "parser.y"
    { 
		if (((OpCodes *)(yyvsp[(1) - (2)]))->lvalue_flag == 2) (yyval) = codes_join3((yyvsp[(1) - (2)]), code_subscript(0), code_dec(1));
		else (yyval) = codes_join((yyvsp[(1) - (2)]), code_dec(1)); 
	;}
    break;

  case 94:
#line 533 "parser.y"
    {
		if (((OpCodes *)(yyvsp[(2) - (2)]))->lvalue_flag == 2) (yyval) = codes_join3((yyvsp[(2) - (2)]), code_subscript(0), code_inc(0));
		else (yyval) = codes_join((yyvsp[(2) - (2)]), code_inc(0));
	;}
    break;

  case 95:
#line 537 "parser.y"
    { 
		if (((OpCodes *)(yyvsp[(2) - (2)]))->lvalue_flag == 2) (yyval) = codes_join3((yyvsp[(2) - (2)]), code_subscript(0), code_dec(0));
		else (yyval) = codes_join((yyvsp[(2) - (2)]), code_dec(0));
	;}
    break;

  case 96:
#line 541 "parser.y"
    { (yyval) = (yyvsp[(2) - (3)]); ;}
    break;

  case 97:
#line 542 "parser.y"
    {
		OpCodes *expr2 = codes_join(code_pop(1), (yyvsp[(3) - (3)]));
		(yyval) = codes_join3((yyvsp[(1) - (3)]), code_jfalse_np(expr2->code_len + 1), expr2);
	;}
    break;

  case 98:
#line 546 "parser.y"
    {
		OpCodes *expr2 = codes_join(code_pop(1), (yyvsp[(3) - (3)]));
		(yyval) = codes_join3((yyvsp[(1) - (3)]), code_jtrue_np(expr2->code_len + 1), expr2);
	;}
    break;

  case 99:
#line 550 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_less()); ;}
    break;

  case 100:
#line 551 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_greater()); ;}
    break;

  case 101:
#line 552 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_lessequ()); ;}
    break;

  case 102:
#line 553 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_greaterequ()); ;}
    break;

  case 103:
#line 554 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_equal()); ;}
    break;

  case 104:
#line 555 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_notequal()); ;}
    break;

  case 105:
#line 556 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_eequ());	;}
    break;

  case 106:
#line 557 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_nneq()); ;}
    break;

  case 107:
#line 558 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_band()); ;}
    break;

  case 108:
#line 559 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_bor()); ;}
    break;

  case 109:
#line 560 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_bxor()); ;}
    break;

  case 110:
#line 561 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_shf(0)); ;}
    break;

  case 111:
#line 562 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_shf(1)); ;}
    break;

  case 112:
#line 563 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_shf(2)); ;}
    break;

  case 113:
#line 564 "parser.y"
    { (yyval) = codes_join3((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_assign(((OpCodes *)(yyvsp[(1) - (3)]))->lvalue_flag)); ;}
    break;

  case 114:
#line 565 "parser.y"
    { (yyval) = opassign((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_add()); ;}
    break;

  case 115:
#line 566 "parser.y"
    { (yyval) = opassign((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_sub()); ;}
    break;

  case 116:
#line 567 "parser.y"
    { (yyval) = opassign((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_mul()); ;}
    break;

  case 117:
#line 568 "parser.y"
    { (yyval) = opassign((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_mod()); ;}
    break;

  case 118:
#line 569 "parser.y"
    { (yyval) = opassign((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_shf(0)); ;}
    break;

  case 119:
#line 570 "parser.y"
    { (yyval) = opassign((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_shf(1)); ;}
    break;

  case 120:
#line 571 "parser.y"
    { (yyval) = opassign((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_shf(2)); ;}
    break;

  case 121:
#line 572 "parser.y"
    { (yyval) = opassign((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_band()); ;}
    break;

  case 122:
#line 573 "parser.y"
    { (yyval) = opassign((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_bor()); ;}
    break;

  case 123:
#line 574 "parser.y"
    { (yyval) = opassign((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_bxor()); ;}
    break;

  case 124:
#line 575 "parser.y"
    { (yyval) = opassign((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), code_div()); ;}
    break;

  case 125:
#line 576 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 126:
#line 578 "parser.y"
    { (yyval) = codes_join((yyvsp[(2) - (2)]), code_newfcall(0)); ;}
    break;

  case 127:
#line 579 "parser.y"
    { 
		if (((OpCodes *)(yyvsp[(2) - (2)]))->lvalue_flag == 2) (yyval) = codes_join3((yyvsp[(2) - (2)]), code_subscript(1), code_newfcall(0));
 		else (yyval) = codes_join((yyvsp[(2) - (2)]), code_newfcall(0));;}
    break;

  case 128:
#line 582 "parser.y"
    { (yyval) = codes_join((yyvsp[(3) - (4)]), code_newfcall(0)); ;}
    break;

  case 129:
#line 583 "parser.y"
    { (yyval) = codes_join((yyvsp[(2) - (2)]), code_newfcall(0)); ;}
    break;

  case 130:
#line 584 "parser.y"
    { 
		OpCodes *opl = (yyvsp[(4) - (5)]);
		int expr_cnt = opl ? opl->expr_counter:0;
 		(yyval) = codes_join3((yyvsp[(2) - (5)]), (opl ? opl : code_nop()), code_newfcall(expr_cnt));
	;}
    break;

  case 131:
#line 589 "parser.y"
    {
		OpCodes *opl = (yyvsp[(4) - (5)]);
		int expr_cnt = opl ? opl->expr_counter:0;
		OpCodes *lv = NULL;
		if (((OpCodes *)(yyvsp[(2) - (5)]))->lvalue_flag == 2) lv = codes_join((yyvsp[(2) - (5)]), code_subscript(1));
		else lv = (yyvsp[(2) - (5)]);
		(yyval) = codes_join3(lv, (opl ? opl : code_nop()), code_newfcall(expr_cnt));
	;}
    break;

  case 132:
#line 597 "parser.y"
    { 
		OpCodes *opl = (yyvsp[(6) - (7)]);
		int expr_cnt = opl ? opl->expr_counter:0;
		(yyval) = codes_join3((yyvsp[(3) - (7)]), (opl ? opl : code_nop()), code_newfcall(expr_cnt));
	;}
    break;

  case 133:
#line 602 "parser.y"
    {
		OpCodes *opl = (yyvsp[(4) - (5)]);
		int expr_cnt = opl ? opl->expr_counter:0;
 		(yyval) = codes_join3((yyvsp[(2) - (5)]), (opl ? opl : code_nop()), code_newfcall(expr_cnt));
	;}
    break;

  case 134:
#line 607 "parser.y"
    {
		OpCodes *expr2 = codes_join((yyvsp[(3) - (5)]), code_jmp(((OpCodes *)(yyvsp[(5) - (5)]))->code_len + 1));
		(yyval) = codes_join4((yyvsp[(1) - (5)]), code_jfalse(expr2->code_len + 1), expr2, (yyvsp[(5) - (5)]));
	;}
    break;

  case 135:
#line 611 "parser.y"
    { (yyval) = codes_join((yyvsp[(3) - (4)]), code_debug()); ;}
    break;

  case 136:
#line 615 "parser.y"
    {
		OpCodes *ff = codes_join4((yyvsp[(1) - (6)]), code_push_string((yyvsp[(3) - (6)])), code_chthis(1), code_subscript(1));
		OpCodes *opl = (yyvsp[(5) - (6)]);
		int expr_cnt = opl ? opl->expr_counter:0;
 		(yyval) = codes_join3(ff, (opl ? opl : code_nop()), code_fcall(expr_cnt));
	;}
    break;

  case 137:
#line 621 "parser.y"
    {
		OpCodes *ff = codes_join4((yyvsp[(1) - (7)]), (yyvsp[(3) - (7)]), code_chthis(1), code_subscript(1));
		OpCodes *opl = (yyvsp[(6) - (7)]);
		int expr_cnt = opl ? opl->expr_counter:0;
 		(yyval) = codes_join3(ff, (opl ? opl : code_nop()), code_fcall(expr_cnt));
	;}
    break;

  case 138:
#line 627 "parser.y"
    {
		OpCodes *opl = (yyvsp[(5) - (6)]);
		int expr_cnt = opl ? opl->expr_counter:0;
 		(yyval) = codes_join4((yyvsp[(2) - (6)]), code_chthis(0), (opl ? opl : code_nop()), code_fcall(expr_cnt));
	;}
    break;

  case 139:
#line 632 "parser.y"
    {
		OpCodes *opl = (yyvsp[(3) - (4)]);
		int expr_cnt = opl ? opl->expr_counter:0;
		OpCodes *pref;
		OpCodes *lval = (yyvsp[(1) - (4)]);
		if (lval->lvalue_flag == 2) {
			pref = codes_join3((yyvsp[(1) - (4)]), code_chthis(1), code_subscript(1));
 			(yyval) = codes_join3(pref, (opl ? opl : code_nop()), code_fcall(expr_cnt));
		} else {
			if (lval->lvalue_name && unistrcmp(lval->lvalue_name, tounichars("eval")) == 0) {
				(yyval) = codes_join((opl ? opl : code_nop()), code_eval(expr_cnt));
			} else {
				pref = codes_join((yyvsp[(1) - (4)]), code_chthis(0));
				(yyval) = codes_join3(pref, (opl ? opl : code_nop()), code_fcall(expr_cnt));
			}
		}
	;}
    break;

  case 140:
#line 652 "parser.y"
    {
		(yyval) = code_push_index((yyvsp[(1) - (1)])); 
		((OpCodes *)(yyval))->lvalue_flag = 1; 
		((OpCodes *)(yyval))->lvalue_name = (yyvsp[(1) - (1)]); 
	;}
    break;

  case 141:
#line 657 "parser.y"
    { (yyval) = code_push_args(); ((OpCodes *)(yyval))->lvalue_flag = 1; ;}
    break;

  case 142:
#line 658 "parser.y"
    { (yyval) = code_push_this(); ((OpCodes *)(yyval))->lvalue_flag = 1; ;}
    break;

  case 143:
#line 659 "parser.y"
    {
		if (((OpCodes *)(yyvsp[(1) - (4)]))->lvalue_flag == 2) (yyval) = codes_join3((yyvsp[(1) - (4)]), code_subscript(1), (yyvsp[(3) - (4)])); 
		else (yyval) = codes_join((yyvsp[(1) - (4)]), (yyvsp[(3) - (4)])); 
		((OpCodes *)(yyval))->lvalue_flag = 2;
	;}
    break;

  case 144:
#line 664 "parser.y"
    {
		if (((OpCodes *)(yyvsp[(1) - (3)]))->lvalue_flag == 2) (yyval) = codes_join3((yyvsp[(1) - (3)]), code_subscript(1), code_push_string((yyvsp[(3) - (3)]))); 
		else (yyval) = codes_join((yyvsp[(1) - (3)]), code_push_string((yyvsp[(3) - (3)])));
		((OpCodes *)(yyval))->lvalue_flag = 2;
	;}
    break;

  case 145:
#line 671 "parser.y"
    { (yyval) = NULL; ;}
    break;

  case 146:
#line 672 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 147:
#line 676 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ((OpCodes *)(yyval))->expr_counter = 1; ;}
    break;

  case 148:
#line 677 "parser.y"
    { 
		int exprcnt = ((OpCodes *)(yyvsp[(1) - (3)]))->expr_counter + 1;
		(yyval) = codes_join((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
		((OpCodes *)(yyval))->expr_counter = exprcnt;
	;}
    break;

  case 149:
#line 684 "parser.y"
    { (yyval) = code_push_string((yyvsp[(1) - (1)])); ;}
    break;

  case 150:
#line 685 "parser.y"
    { (yyval) = code_push_undef(); ;}
    break;

  case 151:
#line 686 "parser.y"
    { (yyval) = code_push_bool(1); ;}
    break;

  case 152:
#line 687 "parser.y"
    { (yyval) = code_push_bool(0); ;}
    break;

  case 153:
#line 688 "parser.y"
    { (yyval) = code_push_num((yyvsp[(1) - (1)])); ;}
    break;

  case 154:
#line 689 "parser.y"
    { (yyval) = code_push_regex((yyvsp[(1) - (1)])); ;}
    break;

  case 155:
#line 690 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 156:
#line 691 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 157:
#line 695 "parser.y"
    { (yyval) = codes_join((yyvsp[(2) - (3)]), code_object(((OpCodes *)(yyvsp[(2) - (3)]))->expr_counter)); ;}
    break;

  case 158:
#line 699 "parser.y"
    { (yyval) = code_nop(); ((OpCodes *)(yyval))->expr_counter = 0; ;}
    break;

  case 159:
#line 700 "parser.y"
    { (yyval) = (yyvsp[(1) - (1)]); ((OpCodes *)(yyval))->expr_counter = 1; ;}
    break;

  case 160:
#line 701 "parser.y"
    {
		int cnt = ((OpCodes *)(yyvsp[(1) - (3)]))->expr_counter + 1;
		(yyval) = codes_join((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
		((OpCodes *)(yyval))->expr_counter = cnt;
	;}
    break;

  case 161:
#line 709 "parser.y"
    { (yyval) = codes_join(code_push_string((yyvsp[(1) - (3)])), (yyvsp[(3) - (3)])); ;}
    break;

  case 162:
#line 710 "parser.y"
    { (yyval) = codes_join(code_push_string((yyvsp[(1) - (3)])), (yyvsp[(3) - (3)])); ;}
    break;

  case 163:
#line 714 "parser.y"
    { (yyval) = codes_join((yyvsp[(2) - (3)]), code_array(((OpCodes *)(yyvsp[(2) - (3)]))->expr_counter)); ;}
    break;

  case 164:
#line 715 "parser.y"
    { (yyval) = code_array(0); ;}
    break;


/* Line 1267 of yacc.c.  */
#line 3328 "parser.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, pstate, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (&yylloc, pstate, yymsg);
	  }
	else
	  {
	    yyerror (&yylloc, pstate, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc, pstate);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, pstate);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, pstate, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, pstate);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, pstate);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 719 "parser.y"



