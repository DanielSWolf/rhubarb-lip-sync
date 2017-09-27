/*
 * regsub
 *
 *	Copyright (c) 1986 by University of Toronto.
 *	Written by Henry Spencer.  Not derived from licensed software.
 *
 *	Permission is granted to anyone to use this software for any
 *	purpose on any computer system, and to redistribute it freely,
 *	subject to the following restrictions:
 *
 *	1. The author is not responsible for the consequences of use of
 *		this software, no matter how awful, even if they arise
 *		from defects in it.
 *
 *	2. The origin of this software must not be misrepresented, either
 *		by explicit claim or by omission.
 *
 *	3. Altered versions must be plainly marked as such, and must not
 *		be misrepresented as being the original software.
 */
/* This is an altered version.  It has been modified by David
   Huggins-Daines <dhd@cepstral.com> on 2001-10-23 to use a different
   API and be re-entrant and safe and all that good stuff. */
#include "cst_regex.h"
#include "cst_string.h"
#include "cst_error.h"

size_t cst_regsub(const cst_regstate *state, const char *in, char *out, size_t max)
{
	const char *src;
	char *dst;
	int c, no, len;
	size_t count;

	if (state == NULL || in == NULL) {
		cst_errmsg("NULL parm to regsub\n");
		cst_error();
	}

	src = in;
	dst = out;
	count = 0;
	while ((c = *src++) != '\0') {
		if (out && dst + 1 > out + max - 1)
			break;
		if (c == '&')
			no = 0;
		else if (c == '\\' && '0' <= *src && *src <= '9')
			no = *src++ - '0';
		else
			no = -1;

 		if (no < 0) {	/* Ordinary character. */
 			if (c == '\\' && (*src == '\\' || *src == '&'))
 				c = *src++;
			if (out)
				*dst++ = c;
			count++;
 		} else if (state->startp[no] != NULL && state->endp[no] != NULL) {
			len = state->endp[no] - state->startp[no];
			if (out) {
				if (dst + len > out + max - 1)
					len = (out + max - 1) - dst;
				strncpy(dst, state->startp[no], len);
				dst += len;
				/* strncpy hit NUL. */
				if (len != 0 && *(dst-1) == '\0') {
					cst_errmsg("damaged match string");
					cst_error();
				}
			}
			count += len;
		}
	}
	if (out && dst - out + 1 < max)
		*dst++ = '\0';

	return count;
}
