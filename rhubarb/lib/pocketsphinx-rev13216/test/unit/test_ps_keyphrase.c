#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "pocketsphinx_internal.h"
#include "test_macros.h"
#include "ps_test.c"

int
main(int argc, char *argv[])
{
    cmd_ln_t *config;

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/en-us/en-us",
                "-keyphrase", "forward",
                "-dict", MODELDIR "/en-us/cmudict-en-us.dict", NULL));
    return ps_decoder_test(config, "KEYPHRASE", "forward");
}
