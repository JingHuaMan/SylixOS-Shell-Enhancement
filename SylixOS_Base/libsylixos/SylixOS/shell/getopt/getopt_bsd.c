/*	$NetBSD: getopt_long.c,v 1.16 2003/10/27 00:12:42 lukem Exp $	*/
/*	$DragonFly: src/lib/libc/stdlib/getopt_long.c,v 1.14 2005/11/20 12:37:48 swildner Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Dieter Baron and Thomas Klausner.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/**************************** HANHUI ***************************/

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"

#if LW_CFG_SHELL_EN > 0

#include "getopt_var.h"
#include "getopt.h"

#define getenv(var)                lib_getenv(var)
#define strchr(str, c)             lib_strchr((PCHAR)str, c)
#define strlen(str)                lib_strlen((PCHAR)str)
#define strncmp(str1, str2, n)     lib_strncmp((PCHAR)str1, (PCHAR)str2, n)
/**************************** HANHUI ***************************/

/* XXX BOOTSTRAPPING */
#ifndef	__DECONST
#define	__DECONST(type, var)	((type)(uintptr_t)(const void *)(var))
#endif

/**************************** HANHUI ***************************/
#if 0
int	opterr = 1;		/* if error message should be printed */
int	optind = 1;		/* index into parent argv vector */
int	optopt = '?';		/* character checked for validity */
int	optreset;		/* reset getopt */
char    *optarg;		/* argument associated with option */
#endif

#define REPLACE_GETOPT
/**************************** HANHUI ***************************/

#define IGNORE_FIRST	((*options == '-') || (*options == '+'))
#define PRINT_ERROR	((opterr) && ((*options != ':') \
				      || (IGNORE_FIRST && (options[1] != ':'))))
#define IS_POSIXLY_CORRECT (getenv("POSIXLY_CORRECT") != NULL)
#define PERMUTE         (!IS_POSIXLY_CORRECT && !IGNORE_FIRST)
/* XXX: GNU ignores PC if *options == '-' */
#define IN_ORDER        (!IS_POSIXLY_CORRECT && (*options == '-'))

/* return values */
#define	BADCH	(int)'?'
#define	BADARG		((IGNORE_FIRST && (options[1] == ':')) \
			 || (*options == ':') ? (int)':' : (int)'?')
#define INORDER (int)1

static int getopt_internal(int, char * const *, const char *, int);
static int getopt_internal_short(int, char * const *, const char *, int);
static int getopt_long_internal(int, char * const *, const char *,
				const struct option *, int *, int);
static int gcd(int, int);
static void permute_args(int, int, int, char * const *);

/**************************** HANHUI ***************************/
PCHAR   *__tshellOptEMsg(VOID);
PCHAR   *__tshellOptPlace(VOID);
INT     *__tshellOptNonoptStart(VOID);
INT     *__tshellOptNonoptEnd(VOID);

#define EMSG            (*__tshellOptEMsg())
#define place           (*__tshellOptPlace())
#define nonopt_start    (*__tshellOptNonoptStart())
#define nonopt_end      (*__tshellOptNonoptEnd())

#if 0
static char EMSG[] = {0};
static char *place = EMSG; /* option letter processing */

/* XXX: set optreset to 1 rather than these two */
static int nonopt_start = -1; /* first non option argument (for permute) */
static int nonopt_end = -1;   /* first option after non options (for permute) */
#endif
/**************************** HANHUI ***************************/

/* Error messages */
/**************************** HANHUI ***************************/

static char *recargchar = "option requires an argument -- %c";
static char *recargstring = "option requires an argument -- %s";
static char *ambig = "ambiguous option -- %.*s";
static char *noarg = "option doesn't take an argument -- %.*s";
static char *illoptchar = "unknown option -- %c";
static char *illoptstring = "unknown option -- %s";

#if 0
static const char recargchar[] = "option requires an argument -- %c";
static const char recargstring[] = "option requires an argument -- %s";
static const char ambig[] = "ambiguous option -- %.*s";
static const char noarg[] = "option doesn't take an argument -- %.*s";
static const char illoptchar[] = "unknown option -- %c";
static const char illoptstring[] = "unknown option -- %s";
#endif
/**************************** HANHUI ***************************/

/*
 * Compute the greatest common divisor of a and b.
 */
static int
gcd(int a, int b)
{
	int c;

	c = a % b;
	while (c != 0) {
		a = b;
		b = c;
		c = a % b;
	}
	   
	return b;
}

/*
 * Exchange the block from nonopt_start to nonopt_end with the block
 * from nonopt_end to opt_end (keeping the same order of arguments
 * in each block).
 */
static void
permute_args(int panonopt_start, int panonopt_end, int opt_end,
	     char * const *nargv)
{
	int cstart, cyclelen, i, j, ncycle, nnonopts, nopts, pos;
	char *swap;

	/*
	 * compute lengths of blocks and number and size of cycles
	 */
	nnonopts = panonopt_end - panonopt_start;
	nopts = opt_end - panonopt_end;
	ncycle = gcd(nnonopts, nopts);
	cyclelen = (opt_end - panonopt_start) / ncycle;

	for (i = 0; i < ncycle; i++) {
		cstart = panonopt_end+i;
		pos = cstart;
		for (j = 0; j < cyclelen; j++) {
			if (pos >= panonopt_end)
				pos -= nnonopts;
			else
				pos += nopts;
			swap = nargv[pos];
			/* LINTED const cast */
			(__DECONST(char **, nargv))[pos] = nargv[cstart];
			/* LINTED const cast */
			(__DECONST(char **, nargv))[cstart] = swap;
		}
	}
}

/*
 * getopt_internal --
 *	Parse argc/argv argument vector.  Called by user level routines.
 *  Returns -2 if -- is found (can be long option or end of options marker).
 */
static int
getopt_internal(int nargc, char * const *nargv, const char *options,
		int long_support)
{
	optarg = NULL;

	/*
	 * XXX Some programs (like rsyncd) expect to be able to
	 * XXX re-initialize optind to 0 and have getopt_long(3)
	 * XXX properly function again.  Work around this braindamage.
	 */
	if (optind == 0)
		optind = 1;

	if (optreset)
		nonopt_start = nonopt_end = -1;
start:
	if (optreset || !*place) {		/* update scanning pointer */
		optreset = 0;
		if (optind >= nargc) {          /* end of argument vector */
			place = EMSG;
			if (nonopt_end != -1) {
				/* do permutation, if we have to */
				permute_args(nonopt_start, nonopt_end,
				    optind, nargv);
				optind -= nonopt_end - nonopt_start;
			}
			else if (nonopt_start != -1) {
				/*
				 * If we skipped non-options, set optind
				 * to the first of them.
				 */
				optind = nonopt_start;
			}
			nonopt_start = nonopt_end = -1;
			return -1;
		}
		place = nargv[optind];
		if ((*place == '-') && (place[1] == '\0') && long_support == 0)
			return -1;
		if ((*place != '-') ||
		    ((*place == '-') && (place[1] == '\0') && long_support != 0)) {
		        /* found non-option */
			place = EMSG;
			if (IN_ORDER) {
				/*
				 * GNU extension: 
				 * return non-option as argument to option 1
				 */
				optarg = nargv[optind++];
				return INORDER;
			}
			if (!PERMUTE) {
				/*
				 * if no permutation wanted, stop parsing
				 * at first non-option
				 */
				return -1;
			}
			/* do permutation */
			if (nonopt_start == -1)
				nonopt_start = optind;
			else if (nonopt_end != -1) {
				permute_args(nonopt_start, nonopt_end,
				    optind, nargv);
				nonopt_start = optind -
				    (nonopt_end - nonopt_start);
				nonopt_end = -1;
			}
			optind++;
			/* process next argument */
			goto start;
		}
		if (nonopt_start != -1 && nonopt_end == -1)
			nonopt_end = optind;
		if (place[1] && *++place == '-') {	/* found "--" */
			if (place[1] == '\0') {
				++optind;
				/*
				 * We found an option (--), so if we skipped
				 * non-options, we have to permute.
				 */
				if (nonopt_end != -1) {
					permute_args(nonopt_start, nonopt_end,
						     optind, nargv);
					optind -= nonopt_end - nonopt_start;
				}
				nonopt_start = nonopt_end = -1;
				return -1;
			} else if (long_support) {
				place++;
				return -2;
			}
		}
	}
	if (long_support == 2 && (place[1] || strchr(options, *place) == NULL))
		return -3;
	return getopt_internal_short(nargc, nargv, options, long_support);
}

static int
getopt_internal_short(int nargc, char * const *nargv, const char *options,
		      int long_support)
{
	const char *oli;			/* option letter list index */
	int optchar;

	if ((optchar = (int)*place++) == (int)':' ||
	    (oli = strchr(options + (IGNORE_FIRST ? 1 : 0), optchar)) == NULL) {
		/* option letter unknown or ':' */
		if (PRINT_ERROR) {
			if (long_support == 2)
				warnx(illoptstring, --place);
			else
				warnx(illoptchar, optchar);
		}
		if (long_support == 2)
			place = EMSG;
		if (*place == 0)
			++optind;
		optopt = optchar;
		return BADCH;
	}
	if (long_support && optchar == 'W' && oli[1] == ';') {
		/* -W long-option */
		if (*place) 
			return -2;

		if (++optind >= nargc) {	/* no arg */
			place = EMSG;
			if (PRINT_ERROR)
				warnx(recargchar, optchar);
			optopt = optchar;
			return BADARG;
		} else				/* white space */
			place = nargv[optind];
		/*
		 * Handle -W arg the same as --arg (which causes getopt to
		 * stop parsing).
		 */
		return -2;
	}
	if (*++oli != ':') {			/* doesn't take argument */
		if (!*place)
			++optind;
	} else {				/* takes (optional) argument */
		optarg = NULL;
		if (*place)			/* no white space */
			optarg = place;
		/* XXX: disable test for :: if PC? (GNU doesn't) */
		else if (oli[1] != ':') {	/* arg not optional */
			if (++optind >= nargc) {	/* no arg */
				place = EMSG;
				if (PRINT_ERROR)
					warnx(recargchar, optchar);
				optopt = optchar;
				return BADARG;
			} else
				optarg = nargv[optind];
		}
		place = EMSG;
		++optind;
	}
	/* dump back option letter */
	return optchar;
}

static int
getopt_long_internal(int nargc, char * const *nargv, const char *options,
		     const struct option *long_options, int *idx, int long_only)
{
	int retval;

	/* idx may be NULL */

	retval = getopt_internal(nargc, nargv, options, long_only ? 2 : 1);
recheck:
	if (retval == -2 || retval == -3) {
		char *current_argv, *has_equal;
		size_t current_argv_len;
		int i, match;

		current_argv = place;
		match = -1;

		optind++;
		place = EMSG;

		if ((has_equal = strchr(current_argv, '=')) != NULL) {
			/* argument found (--option=arg) */
			current_argv_len = has_equal - current_argv;
			has_equal++;
		} else
			current_argv_len = strlen(current_argv);
	    
		for (i = 0; long_options[i].name; i++) {
			/* find matching long option */
			if (strncmp(current_argv, long_options[i].name,
			    current_argv_len))
				continue;

			if (strlen(long_options[i].name) ==
			    (unsigned)current_argv_len) {
				/* exact match */
				match = i;
				break;
			}
			if (match == -1)		/* partial match */
				match = i;
			else {
				/* ambiguous abbreviation */
				if (PRINT_ERROR)
					warnx(ambig, (int)current_argv_len,
					     current_argv);
				optopt = 0;
				return BADCH;
			}
		}
		if (match != -1) {			/* option found */
		        if (long_options[match].has_arg == no_argument
			    && has_equal) {
				if (PRINT_ERROR)
					warnx(noarg, (int)current_argv_len,
					     current_argv);
				/*
				 * XXX: GNU sets optopt to val regardless of
				 * flag
				 */
				if (long_options[match].flag == NULL)
					optopt = long_options[match].val;
				else
					optopt = 0;
				return BADARG;
			}
			if (long_options[match].has_arg == required_argument ||
			    long_options[match].has_arg == optional_argument) {
				if (has_equal)
					optarg = has_equal;
				else if (long_options[match].has_arg ==
				    required_argument) {
					/*
					 * optional argument doesn't use
					 * next nargv
					 */
					optarg = nargv[optind++];
				}
			}
			if ((long_options[match].has_arg == required_argument)
			    && (optarg == NULL)) {
				/*
				 * Missing argument; leading ':'
				 * indicates no error should be generated
				 */
				if (PRINT_ERROR)
					warnx(recargstring, current_argv);
				/*
				 * XXX: GNU sets optopt to val regardless
				 * of flag
				 */
				if (long_options[match].flag == NULL)
					optopt = long_options[match].val;
				else
					optopt = 0;
				--optind;
				return BADARG;
			}
		} else if (retval == -3) {
			--optind;
			place = current_argv;
			retval = getopt_internal_short(nargc, nargv,
			    options, long_only ? 2 : 1);
			goto recheck;
		} else {			/* unknown option */
			if (PRINT_ERROR)
				warnx(illoptstring, current_argv);
			optopt = 0;
			return BADCH;
		}
		if (long_options[match].flag) {
			*long_options[match].flag = long_options[match].val;
			retval = 0;
		} else 
			retval = long_options[match].val;
		if (idx)
			*idx = match;
	}
	return retval;
}

#ifdef REPLACE_GETOPT
/*
 * getopt --
 *	Parse argc/argv argument vector.
 *
 * [eventually this will replace the real getopt]
 */
int
getopt(int nargc, char * const *nargv, const char *options)
{
	return getopt_internal(nargc, nargv, options, 0);
}
#endif

/*
 * getopt_long --
 *	Parse argc/argv argument vector.
 */

int
getopt_long(int nargc, char * const *nargv, const char *options,
	    const struct option *long_options, int *idx)
{
	return getopt_long_internal(nargc, nargv, options, long_options,
				    idx, 0);
}

/*
 * getopt_long_only --
 *	Parse argc/argv argument vector.
 *	Prefers long options over short options for single dash arguments.
 */

int
getopt_long_only(int nargc, char * const *nargv, const char *options,
		 const struct option *long_options, int *idx)
{
	return getopt_long_internal(nargc, nargv, options, long_options,
				    idx, 1);
}

#endif  /*  LW_CFG_SHELL_EN > 0         */
