#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "pocketsphinx_internal.h"
#include "test_macros.h"
#include "ps_test.c"

int
main(int argc, char *argv[])
{
	ps_decoder_t *ps;
	ps_nbest_t *nbest;
	cmd_ln_t *config;
	FILE *rawfh;
	char const *hyp;
	int32 score, n;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/en-us/en-us",
				"-lm", MODELDIR "/en-us/en-us.lm.bin",
				"-dict", MODELDIR "/en-us/cmudict-en-us.dict",
				"-fwdtree", "yes",
				"-fwdflat", "yes",
				"-bestpath", "yes",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));
	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
	ps_decode_raw(ps, rawfh, -1);
	fclose(rawfh);
	hyp = ps_get_hyp(ps, &score);
	printf("BESTPATH: %s (%d)\n", hyp, score);

	for (n = 1, nbest = ps_nbest(ps); nbest && n < 10; nbest = ps_nbest_next(nbest), n++) {
		ps_seg_t *seg;
		hyp = ps_nbest_hyp(nbest, &score);
		printf("NBEST %d: %s (%d)\n", n, hyp, score);
		for (seg = ps_nbest_seg(nbest); seg;
		     seg = ps_seg_next(seg)) {
			char const *word;
			int sf, ef;

			word = ps_seg_word(seg);
			ps_seg_frames(seg, &sf, &ef);
			printf("%s %d %d\n", word, sf, ef);
		}
	}
	if (nbest)
	    ps_nbest_free(nbest);
	ps_free(ps);
	cmd_ln_free_r(config);
	return 0;
}
