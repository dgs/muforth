/*
 * $Id$
 *
 * This file is part of muforth.
 *
 * Copyright (c) 1997-2004 David Frech. All rights reserved, and all wrongs
 * reversed.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * or see the file LICENSE in the top directory of this distribution.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* dictionary */

#include "muforth.h"
#include <stdio.h>

struct dict_entry
{
    struct dict_entry *link;
    void *code;
    unsigned char length;
    char name[0];
};

/* the forth and compiler "vocabulary" chains */
struct dict_entry *forth_chain = NULL;
struct dict_entry *compiler_chain = NULL;

/* current chain to compile into */
struct dict_entry **current_chain = &forth_chain;

void (*mu_name_hook)() = mu_nope;	/* called when a name is created */

/* bogus C-style dictionary init */
struct inm			/* "initial name" */
{
    char *name;
    void (*code)();
};

struct inm initial_forth[] = {
    { "version", mu_push_version },
    { "build-time", mu_push_build_time },
    { ":", mu_colon },
    { "name", mu_make_new_name },
    { "'name-hook", mu_push_tick_name_hook },
    { ".forth.", mu_push_forth_chain },
    { ".compiler.", mu_push_compiler_chain },
    { "current", mu_push_current },
    { "'number", mu_push_tick_number },
    { "'number,", mu_push_tick_number_comma },
    { "h", mu_push_h },
    { "r", mu_push_r },
    { "s0", mu_push_s0 },
    { "sp", mu_push_sp },
    { "catch", mu_catch },
    { "throw", mu_throw },
    { "command-line", mu_push_command_line },
    { "token", mu_token },
    { "parse", mu_parse },
    { "find", mu_find },
    { "interpret", mu_interpret },
    { "evaluate", mu_evaluate },
    { "create-file", mu_create_file },
    { "open-file", mu_open_file },
    { "r/o", mu_push_ro_flags },
    { "r/w", mu_push_rw_flags },
    { "close-file", mu_close_file },
    { "mmap-file", mu_mmap_file },    
    { "load-file", mu_load_file },
    { "readable?", mu_readable_q },
    { "load-literal", mu_compile_literal_load },
    { "push-literal", mu_compile_literal_push },
    { "fetch-literal", mu_fetch_literal_value },
    { "compile,", mu_compile_call },
    { "resolve", mu_resolve },
    { "state", mu_push_state },
    { "-]", mu_minus_rbracket },
    { "parsed", mu_push_parsed },
    { "huh?", mu_huh },
    { "complain", mu_complain },
    { "(0branch)", mu_compile_destructive_zbranch },
    { "(=0branch)", mu_compile_nondestructive_zbranch },
    { "(branch)", mu_compile_branch },
    { "(?for)", mu_compile_qfor },
    { "(next)", mu_compile_next },
    { "scrabble", mu_scrabble },
    { "nope", mu_nope },
    { "zzz", mu_zzz },
    { "+", mu_add },
    { "and", mu_and },
    { "or", mu_or },
    { "xor", mu_xor },
    { "negate", mu_negate },
    { "invert", mu_invert },
    { "u<", mu_uless },
    { "0<", mu_zless },
    { "0=", mu_zequal },
    { "<", mu_less },
    { "2*", mu_two_star },
    { "2/", mu_two_slash },
    { "u2/", mu_two_slash_unsigned },
    { "<<", mu_shift_left },
    { ">>", mu_shift_right },
    { "u>>", mu_shift_right_unsigned },
    { "d+", mu_dplus },
    { "dnegate", mu_dnegate },
    { "um*", mu_um_star },
    { "m*", mu_m_star },
    { "um/mod", mu_um_slash_mod },
    { "fm/mod", mu_fm_slash_mod },
    { "@", mu_fetch },
    { "c@", mu_cfetch },
    { "!", mu_store },
    { "c!", mu_cstore },
    { "+!", mu_plus_store },
    { "rot", mu_rot },
    { "-rot", mu_minus_rot },
    { "dup", mu_dupe },
    { "nip", mu_nip },
    { "swap", mu_swap },
    { "over", mu_over },
    { "tuck", mu_tuck },
    { "string-compare", mu_string_compare },
    { "read", mu_read_carefully },
    { "write", mu_write_carefully },
    { "sp@", mu_sp_fetch },
    { "sp!", mu_sp_store },
    { "cmove", mu_cmove },
    { "cells", mu_cells },
    { "cell/", mu_cell_slash },

    /* time.c */
    { "local-time", mu_local_time },
    { "utc", mu_global_time },
    { "clock", mu_push_clock },

#ifdef __FreeBSD__
    /* pci.c */
    { "pci", mu_pci_open },
    { "pci@", mu_pci_read },
#endif

    /* tty.c */
    { "get-termios", mu_get_termios },
    { "set-termios", mu_set_termios },
    { "set-raw", mu_set_termios_raw },
    { "set-min-time", mu_set_termios_min_time },
    { "set-speed", mu_set_termios_speed },

    /* select.c */
    { "fd-zero", mu_fd_zero },
    { "fd-set", mu_fd_set },
    { "fd-clr", mu_fd_clr },
    { "fd-in-set?", mu_fd_isset },
    { "select", mu_select },

    /* sort.c */
    { "string-quicksort", mu_string_quicksort },

    /* i386_lib.s */
    { "jump", mu_jump },

    { "bye", mu_bye },

    { "#code", mu_push_code_size},
    { "#data", mu_push_data_size},
    { "#name", mu_push_name_size},
    { NULL, NULL }
};

struct inm initial_compiler[] = {
    { ";", mu_semicolon },
    { "^", mu_compile_exit },
    { "[", mu_lbracket },
    { "drop", mu_compile_drop },
    { "2drop", mu_compile_2drop },
    { "push", mu_compile_push_to_r },
    { "2push", mu_compile_2push_to_r },
    { "pop", mu_compile_pop_from_r },
    { "2pop", mu_compile_2pop_from_r },
    { "r@", mu_compile_copy_from_r },
    { "shunt", mu_compile_shunt },
    { "execute", mu_compile_push_to_r },
    { NULL, NULL }
};

void mu_push_current()
{
    PUSH(&current_chain);
}

void mu_push_forth_chain()
{
    PUSH(&forth_chain);
}

void mu_push_compiler_chain()
{
    PUSH(&compiler_chain);
}

/* 2004-apr-01. After giving a talk on muforth to SVFIG, and in particular
 * after Randy asked me some pointed questions, I decided that find should
 * have positive logic after all. I have renamed -"find to find to indicate
 * this change.
 */
/* find  ( a u chain - a u 0 | code -1) */
void mu_find()
{
    char *token = (char *) STK(2);
    cell_t length = STK(1);
    struct dict_entry *pde = (struct dict_entry *) TOP;

    while ((pde = pde->link) != NULL)
    {
	if (pde->length != length) continue;
	if (memcmp(pde->name, token, length) != 0) continue;

	/* found: drop token, push code address and true flag */
	STK(2) = (cell_t) pde->code;
	STK(1) = -1;
	DROP(1);
	return;
    }
    /* not found: leave token, push false */
    TOP = 0;
}

static void compile_dict_entry(
    struct dict_entry **ppde, char *name, int length, void *pcode)
{
    struct dict_entry *pde = (struct dict_entry *) pnm;

    pde->link = *ppde;		/* compile link to last */
    *ppde = pde;		/* link onto front of chain */
    pde->code = pcode;		/* compile code pointer */
    pde->length = length;
    memcpy(pde->name, name, length);	/* copy name string */
    pnm = (uint8_t *)ALIGNED(pde->name + length);

#if defined(BEING_DEFINED)
    printf("%*s: %p\n", length, pde->name, pcode);
#endif /* BEING_DEFINED */
}

/* called from Forth */
void mu_compile_name()
{
    compile_dict_entry(current_chain, (char *) STK(1), STK(0), pcd);
    DROP(2);
}

void mu_make_new_name()
{
    mu_token();
    (*mu_name_hook)();
    mu_compile_name();
}

void mu_push_tick_name_hook()
{
    PUSH(&mu_name_hook);
}
    
/* 
We're going to take some pointers from Martin Tracy and zenFORTH.
`last-link' is a 2variable that stores (link, chain) of the last
named word defined.  We don't actually link it into the dictionary
until we think it's safe to do so, thereby obviating the need for
`smudge'.

2variable last-link  ( &link &chain)
variable last-code   ( pointer to last-compiled code field)
variable last-word   ( last compiled word)

: show     last-link 2@  ( &link chain)  !   ; 
: use      ( code -)  last-code @ !  ;
: patch	   pop @  use	; 

: ?unique  ( a u - a u)
   2dup current @  ?unique-hook  find  if
   drop  2dup  fd-out @ push  >stderr  type  ."  again.  "  pop writes 
   then   2drop ;

: code,   0 , ( lex)  here last-code !  0 , ( code)  ;
: token,  ( - 'link)  token  ?unique  here  scrabble>  allot  ;
: link,   ( here)  current @  dup @ ,  last-link 2!  ;
: head,   token,  link,  ;

: name        head,  code,  ;
: noname   0 align,  code,  ;

( Dictionary structure)
: link>name   1- ( char-)  dup c@  swap over -  swap  ;
: >link  ( body - link)  cell- cell- cell- ;
: link>  ( link - body)  cell+ cell+ cell+ ;
: >name  ( body - a u)   >link  link>name  ;
: >lex   ( body - lex)   cell- cell-  ;
: >code  ( body - code)  cell-  ;
*/

static void init_chain(struct dict_entry **pchain, struct inm *pinm)
{
    for (; pinm->name != NULL; pinm++)
	compile_dict_entry(pchain, pinm->name, strlen(pinm->name), pinm->code);
}

void init_dict()
{
    init_chain(&forth_chain, initial_forth);
    init_chain(&compiler_chain, initial_compiler);
}



