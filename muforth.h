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

#include <sys/types.h>
#include <string.h>

/*
 * Multi-architecture support
 *
 * We need #defines for things that vary among the architectures that
 * muforth compiles on. Although so far the two architectures supported are
 * both 32 bits, this may change.
 */
#ifdef __APPLE__
typedef unsigned char uint8_t;
#endif /* __APPLE__ */

#ifdef __POWERPC__
typedef int32_t   cell_t;
typedef uint32_t  code_t;
#define CELL_S    32
#else
#ifdef __i386__
typedef int32_t   cell_t;
typedef uint8_t   code_t;
#define CELL_S    32
#else
#  error "muforth won't build on your architecture!"
#endif
#endif

/* data stack */
#define STACK_SIZE 4096
#define STACK_SAFETY 256
#define S0 &stack[STACK_SIZE - STACK_SAFETY]

/* gcc generates stupid code using this def'n */
/* #define PUSH(n) 	(*--sp = (cell_t)(n)) */
#define PUSH(n)		(sp[-1] = (cell_t)(n), --sp)
#define POP		(*sp++)
#define STK(n)  	(sp[n])
#define TOP		STK(0)
#define DROP(n)		(sp += n)
#ifdef __i386__
#define EXECUTE		i386_execute()
#else
#define EXECUTE		(*(void (*)()) POP)()
#endif

#ifdef DEBUG
#include <stdio.h>
#define BUG printf
#else
#define BUG
#endif

#define ALIGN_SIZE	sizeof(cell_t)
#define ALIGNED(x)	(((cell_t)(x) + ALIGN_SIZE - 1) & -ALIGN_SIZE)

/*
 * struct string is a "normal" string: pointer to the first character, and
 * length. However, since these are often sitting on the data stack with
 * the length on top (and therefore at a _lower_ address) let's define it
 * that way.
 */
struct string
{
    size_t length;
    char *data;
};

/*
 * struct text is an odd beast. It's intended for parsing, and other
 * applications that scan a piece of text. To make this more efficient
 * we store a pointer to the _end_ of the text, and a _negative_
 * offset to its start, rather than the way struct string works.
 */

struct text
{
    char *end;
    ssize_t start;	/* ssize_t is a _signed_ type */
};

struct counted_string
{
    size_t length;	/* cell-sized length, unlike older Forths */
    char data[0];
};

#define COUNTED_STRING(x)	{ strlen(x), x }

extern struct string parsed;	/* for errors */

extern cell_t stack[];
extern cell_t *sp;

extern int  cmd_line_argc;
extern char **cmd_line_argv;

extern uint8_t *pnm0, *pdt0;	/* ptrs to name & data spaces */
extern code_t  *pcd0;		/* ptr to code space */

extern uint8_t *pnm, *pdt;    /* ptrs to next free byte in each space */
extern code_t  *pcd;

extern void (*mu_number)();
extern void (*mu_number_comma)();
extern void (*mu_name_hook)();		/* called when a name is created */

/* XXX: Gross hack alert! */
extern char *ate_the_stack;
extern char *isnt_defined;
extern char *version;

/* declare common functions */
/* muforth.c */
void mu_push_name_size(void);
void mu_push_code_size(void);
void mu_push_data_size(void);
void mu_push_command_line(void);
void mu_push_version(void);
void mu_push_build_time(void);

/* error.c */
void die(const char *msg);
void mu_catch(void);
void mu_throw(void);
char *counted_strerror(void);

/* file.c */
void mu_create_file(void);
void mu_open_file(void);
void mu_push_ro_flags(void);
void mu_push_rw_flags(void);
void mu_close_file(void);
void mu_mmap_file(void);
void mu_load_file(void);
void mu_readable_q();
void mu_read_carefully(void);  /* XXX: temporary */
void mu_write_carefully(void); /* XXX: temporary */

/* i386.c */
void i386_execute(void);
void mu_compile_call(void);
void mu_resolve(void);
void mu_compile_jump(void);
void mu_compile_entry(void);
void mu_compile_exit(void);
void mu_compile_drop(void);
void mu_compile_2drop(void);
void mu_compile_inline_literal(void);
void mu_compile_split_literal_load(void);
void mu_compile_split_literal_push(void);
void mu_fetch_literal_value(void);
void mu_compile_destructive_zbranch();
void mu_compile_nondestructive_zbranch();
void mu_compile_branch(void);
void mu_compile_push_to_r(void);
void mu_compile_2push_to_r(void);
void mu_compile_shunt(void);
void mu_compile_pop_from_r(void);
void mu_compile_2pop_from_r(void);
void mu_compile_copy_from_r(void);
void mu_compile_qfor(void);
void mu_compile_next(void);
void mu_add(void);
void mu_and(void);
void mu_or(void);
void mu_xor(void);
void mu_negate(void);
void mu_invert(void);
void mu_two_star(void);
void mu_two_slash(void);
void mu_two_slash_unsigned(void);
void mu_fetch(void);
void mu_cfetch(void);
void mu_dupe(void);
void mu_nip(void);
void mu_drop(void);
void mu_two_drop(void);

/* i386_lib.s */
/*
void i386_lib_start(void);
void i386_lib_end(void);
extern code_t *i386_into_cee;
*/
extern code_t *i386_lib_start;
extern code_t *i386_lib_end;
void i386_into_cee(void);
void i386_into_forth(void);
void mu_dplus(void);
void mu_dnegate(void);
void mu_um_star(void);
void mu_m_star(void);
void mu_um_slash_mod(void);
void mu_sm_slash_rem(void);
void mu_fm_slash_mod(void);
void mu_jump(void);

/* interpret.c */
void mu_start_up(void);
void mu_nope(void);
void mu_zzz(void);
void mu_token(void);
void mu_parse(void);
void mu_huh(void);
void mu_complain(void);
void mu_depth(void);
void mu_interpret(void);
void mu_evaluate(void);
void mu_push_tick_number(void);
void mu_push_tick_number_comma(void);
void mu_push_state(void);
void mu_lbracket(void);
void mu_minus_rbracket(void);
void mu_push_parsed(void);
void mu_bye(void);

/* compile.c */
void mu_push_h(void);
void mu_push_r(void);
void mu_push_s0(void);
void mu_push_sp(void);
void mu_scrabble(void);
void mu_colon(void);
void mu_semicolon(void);
char *to_counted_string(char *);

/* dict.c */
void mu_definitions(void);
void mu_push_forth_chain(void);
void mu_push_compiler_chain(void);
void mu_push_current(void);
void mu_find(void);
void mu_show(void);
void mu_make_new_name(void);
void mu_push_tick_name_hook(void);
void init_dict(void);

/* buf.c */

/* kernel.c */
void mu_shift_left(void);
void mu_shift_right(void);
void mu_shift_right_unsigned(void);
void mu_store(void);
void mu_cstore(void);
void mu_plus_store(void);
void mu_rot(void);
void mu_minus_rot(void);
void mu_swap(void);
void mu_over(void);
void mu_tuck(void);
void mu_string_compare(void);
int string_compare(const char *string1, size_t length1,
		   const char *string2, size_t length2);
void mu_uless(void);
void mu_zless(void);
void mu_zequal(void);
void mu_less(void);
void mu_sp_fetch(void);
void mu_sp_store(void);
void mu_cmove(void);
void mu_dplus(void);
void mu_dnegate(void);

/* time.c */
void mu_local_time(void);
void mu_global_time(void);
void mu_push_clock(void);

/* pci.c */
#ifdef __FreeBSD__
void mu_pci_read(void);
void mu_pci_open(void);
#endif

/* tty.c */
void mu_get_termios(void);
void mu_set_termios(void);
void mu_set_termios_raw(void);
void mu_set_termios_min_time(void);
void mu_set_termios_speed(void);

/* select.c */
void mu_fd_zero(void);
void mu_fd_set(void);
void mu_fd_clr(void);
void mu_fd_isset(void);
void mu_select(void);

/* sort.c */
void mu_string_quicksort(void);

