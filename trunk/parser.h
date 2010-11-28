/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

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


