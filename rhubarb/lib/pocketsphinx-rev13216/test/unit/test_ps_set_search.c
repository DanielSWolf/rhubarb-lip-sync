#include <sphinxbase/jsgf.h>

#include <pocketsphinx.h>
#include <pocketsphinx_internal.h>

#include "test_macros.h"

static cmd_ln_t *
default_config()
{
    return cmd_ln_init(NULL, ps_args(), TRUE,
                       "-hmm", MODELDIR "/en-us/en-us",
                       "-dict", MODELDIR "/en-us/cmudict-en-us.dict", NULL);
}

static void
test_no_search()
{
    cmd_ln_t *config = default_config();
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(ps_start_utt(ps) < 0);
    ps_free(ps);
    cmd_ln_free_r(config);
}

static void
test_default_fsg()
{
    cmd_ln_t *config = default_config();
    cmd_ln_set_str_r(config, "-hmm", DATADIR "/tidigits/hmm");
    cmd_ln_set_str_r(config, "-dict", DATADIR "/tidigits/lm/tidigits.dic");
    cmd_ln_set_str_r(config, "-fsg", DATADIR "/tidigits/lm/tidigits.fsg");
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(!ps_get_lm(ps, PS_DEFAULT_SEARCH));
    TEST_ASSERT(ps_get_fsg(ps, PS_DEFAULT_SEARCH));
    ps_free(ps);
    cmd_ln_free_r(config);
}

static void
test_default_jsgf()
{
    cmd_ln_t *config = default_config();
    cmd_ln_set_str_r(config, "-jsgf", DATADIR "/goforward.gram");
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(!ps_get_lm(ps, PS_DEFAULT_SEARCH));
    TEST_ASSERT(ps_get_fsg(ps, PS_DEFAULT_SEARCH));
    ps_free(ps);
    cmd_ln_free_r(config);
}

static void
test_default_lm()
{
    cmd_ln_t *config = default_config();
    cmd_ln_set_str_r(config, "-lm", MODELDIR "/en-us/en-us.lm.bin");
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(!ps_get_fsg(ps, PS_DEFAULT_SEARCH));
    TEST_ASSERT(ps_get_lm(ps, PS_DEFAULT_SEARCH));
    ps_free(ps);
    cmd_ln_free_r(config);
}

static void
test_default_lmctl()
{
    cmd_ln_t *config = default_config();
    cmd_ln_set_str_r(config, "-lmctl", DATADIR "/test.lmctl");
    cmd_ln_set_str_r(config, "-lmname", "tidigits");
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(ps_get_lm(ps, "tidigits"));
    TEST_ASSERT(ps_get_lm(ps, "turtle"));
    TEST_ASSERT(!ps_set_search(ps, "turtle"));
    TEST_ASSERT(!ps_set_search(ps, "tidigits"));
    ps_free(ps);
    cmd_ln_free_r(config);
}

static void
test_set_search()
{
    cmd_ln_t *config = default_config();
    ps_decoder_t *ps = ps_init(config);
    ps_search_iter_t *itor;

    jsgf_t *jsgf = jsgf_parse_file(DATADIR "/goforward.gram", NULL);
    fsg_model_t *fsg = jsgf_build_fsg(jsgf,
                                      jsgf_get_rule(jsgf, "goforward.move"),
                                      ps->lmath, cmd_ln_int32_r(config, "-lw"));
    TEST_ASSERT(!ps_set_fsg(ps, "goforward", fsg));
    fsg_model_free(fsg);
    jsgf_grammar_free(jsgf);

    TEST_ASSERT(!ps_set_jsgf_file(ps, "goforward_other", DATADIR "/goforward.gram"));
    // Second time
    TEST_ASSERT(!ps_set_jsgf_file(ps, "goforward_other", DATADIR "/goforward.gram"));

    ngram_model_t *lm = ngram_model_read(config, DATADIR "/tidigits/lm/tidigits.lm.bin",
                                         NGRAM_AUTO, ps->lmath);
    TEST_ASSERT(!ps_set_lm(ps, "tidigits", lm));
    ngram_model_free(lm);

    TEST_ASSERT(!ps_set_search(ps, "tidigits"));

    TEST_ASSERT(!ps_set_search(ps, "goforward"));
    
    itor = ps_search_iter(ps);
    TEST_EQUAL(0, strcmp("goforward_other", ps_search_iter_val(itor)));
    itor = ps_search_iter_next(itor);
    TEST_EQUAL(0, strcmp("tidigits", ps_search_iter_val(itor)));
    itor = ps_search_iter_next(itor);
    TEST_EQUAL(0, strcmp("goforward", ps_search_iter_val(itor)));
    itor = ps_search_iter_next(itor);
    TEST_EQUAL(0, strcmp("_default_pl", ps_search_iter_val(itor)));
    itor = ps_search_iter_next(itor);
    TEST_EQUAL(NULL, itor);
    
    TEST_ASSERT(!ps_start_utt(ps));
    TEST_ASSERT(!ps_end_utt(ps));
    ps_free(ps);
    cmd_ln_free_r(config);
}

static void
test_check_mode()
{
    cmd_ln_t *config = default_config();
    ps_decoder_t *ps = ps_init(config);

    TEST_ASSERT(!ps_set_jsgf_file(ps, "goforward", DATADIR "/goforward.gram"));

    ngram_model_t *lm = ngram_model_read(config, DATADIR "/tidigits/lm/tidigits.lm.bin",
                                         NGRAM_AUTO, ps->lmath);
    TEST_ASSERT(!ps_set_lm(ps, "tidigits", lm));
    ngram_model_free(lm);

    TEST_ASSERT(!ps_set_search(ps, "tidigits"));

    ps_start_utt(ps);
    TEST_EQUAL(-1, ps_set_search(ps, "tidigits"));
    TEST_EQUAL(-1, ps_set_search(ps, "goforward"));    
    ps_end_utt(ps);
    
    ps_free(ps);
    cmd_ln_free_r(config);
}

int
main(int argc, char* argv[])
{
    test_no_search();
    test_default_fsg();
    test_default_jsgf();
    test_default_lm();
    test_default_lmctl();
    test_set_search();
    test_check_mode();

    return 0;
}


