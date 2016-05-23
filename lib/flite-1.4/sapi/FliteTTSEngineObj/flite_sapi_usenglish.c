/*************************************************************************/
/*                                                                       */
/*                           Cepstral, LLC                               */
/*                        Copyright (c) 2001                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CEPSTRAL, LLC AND THE CONTRIBUTORS TO THIS WORK DISCLAIM ALL         */
/*  WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED       */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL         */
/*  CEPSTRAL, LLC NOR THE CONTRIBUTORS BE LIABLE FOR ANY SPECIAL,        */
/*  INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER          */
/*  RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION    */
/*  OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR  */
/*  IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.          */
/*                                                                       */
/*************************************************************************/
/*             Author:  David Huggins-Daines (dhd@cepstral.com)          */
/*               Date:  December 2001                                    */
/*************************************************************************/
/*                                                                       */
/* flite_sapi_usenglish.c: US English specific functions for Flite/SAPI  */
/*                                                                       */
/*************************************************************************/

#include "flite_sapi_usenglish.h"
#include "cst_string.h"

static const char *sapi_usenglish_phones[] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, "pau", "1",
	"2", "aa", "ae", "ah", "ao", "aw", "ax", "ay", "b",
	"ch", "d", "dh", "eh", "er", "ey", "f", "g", "hh", "ih",
	"iy", "jh", "k", "l", "m", "n", "ng", "ow", "oy", "p",
	"r", "s", "sh", "t", "th", "uh", "uw", "v", "w", "y",
	"z", "zh"
};
static const int sapi_usenglish_nphones = (sizeof(sapi_usenglish_phones)
					   / sizeof(sapi_usenglish_phones[0]));

static const char *vis0[] = {"pau", NULL};
static const char *vis1[] = {"ae", "ax", "ah", NULL};
static const char *vis2[] = {"aa", NULL};
static const char *vis3[] = {"ao", NULL};
static const char *vis4[] = {"ey", "eh", "uh", NULL};
static const char *vis5[] = {"er", NULL};
static const char *vis6[] = {"y", "iy", "ih", NULL};
static const char *vis7[] = {"w", "uw", NULL};
static const char *vis8[] = {"ow", NULL};
static const char *vis9[] = {"aw", NULL};
static const char *vis10[] = {"oy", NULL};
static const char *vis11[] = {"ay", NULL};
static const char *vis12[] = {"hh", NULL};
static const char *vis13[] = {"r", NULL};
static const char *vis14[] = {"l", NULL};
static const char *vis15[] = {"s", "z", NULL};
static const char *vis16[] = {"sh", "ch", "jh", "zh", NULL};
static const char *vis17[] = {"th", "dh", NULL};
static const char *vis18[] = {"f", "v", NULL};
static const char *vis19[] = {"d", "t", "n", NULL};
static const char *vis20[] = {"k", "g", "ng", NULL};
static const char *vis21[] = {"p", "b", "m", NULL};

static const struct {
	int vis;
	const char *const *phones;
} sapi_usenglish_visemes[] = {
	{ SP_VISEME_0, vis0 },
	{ SP_VISEME_1, vis1 },
	{ SP_VISEME_2, vis2 },
	{ SP_VISEME_3, vis3 },
	{ SP_VISEME_4, vis4 },
	{ SP_VISEME_5, vis5 },
	{ SP_VISEME_6, vis6 },
	{ SP_VISEME_7, vis7 },
	{ SP_VISEME_8, vis8 },
	{ SP_VISEME_9, vis9 },
	{ SP_VISEME_10, vis10 },
	{ SP_VISEME_11, vis11 },
	{ SP_VISEME_12, vis12 },
	{ SP_VISEME_13, vis13 },
	{ SP_VISEME_14, vis14 },
	{ SP_VISEME_15, vis15 },
	{ SP_VISEME_16, vis16 },
	{ SP_VISEME_17, vis17 },
	{ SP_VISEME_18, vis18 },
	{ SP_VISEME_19, vis19 },
	{ SP_VISEME_20, vis20 },
	{ SP_VISEME_21, vis21 },
	{ -1, NULL }
};

int
flite_sapi_usenglish_phoneme(cst_item *s)
{
	const char *name;
	int i;

	if (s == NULL)
		return 0;

	name = item_name(s);
	for (i = 0; i < sapi_usenglish_nphones; ++i)
		if (sapi_usenglish_phones[i]
		    && cst_streq(sapi_usenglish_phones[i], name))
			break;
	if (i == sapi_usenglish_nphones)
		return 0;
	return i;
}

int
flite_sapi_usenglish_viseme(cst_item *s)
{
	const char *name;
	int i;

	/* FIXME: Is 0 a valid viseme or not? */
	if (s == NULL)
		return -1;

	name = item_name(s);
	for (i = 0; sapi_usenglish_visemes[i].phones; ++i)
		if (cst_member_string(name, sapi_usenglish_visemes[i].phones))
			break;
	return sapi_usenglish_visemes[i].vis;
}

int
flite_sapi_usenglish_feature(cst_item *s)
{
	int feat = 0;

	if (s == NULL)
		return 0;

	if (ffeature_int(s, "R:SylStructure.parent.stress"))
		feat |= SPVFEATURE_STRESSED;
	if (ffeature_int(s, "R:SylStructure.parent.accented"))
		feat |= SPVFEATURE_EMPHASIS;

	return feat;
}

cst_val *
flite_sapi_usenglish_pronounce(SPPHONEID *pids)
{
	SPPHONEID *p;
	cst_val *phones = NULL;

	for (p = pids; *p; ++p) {
		const char *pname;

		if (*p >= sapi_usenglish_nphones)
			continue;
		pname = sapi_usenglish_phones[*p];
		if (pname == NULL)
			continue;
		if (isdigit(pname[0])) {
			/* stress numbers */
			if (phones) {
				char *psname;
				cst_val *car, *cdr;

				car = (cst_val *) val_car(phones);
				cdr = (cst_val *) val_cdr(phones);
				psname = cst_alloc(char,
						   strlen(val_string(car))
						   + strlen(pname) + 1);
				sprintf(psname, "%s%s", val_string(car), pname);
				phones = cons_val(string_val(psname), cdr);
				cst_free(psname);
				delete_val(car);
			}
		} else
			phones = cons_val(string_val(pname), phones);
	}
	return val_reverse(phones);
}
