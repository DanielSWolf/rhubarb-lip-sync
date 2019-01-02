/* simple test program to print out the utf8proc properties for a codepoint */

#include "tests.h"

int main(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; ++i) {
        utf8proc_uint8_t cstr[16], *map;
        unsigned int c;
        if (!strcmp(argv[i], "-V")) {
            printf("utf8proc version %s\n", utf8proc_version());
            continue;
        }
        check(sscanf(argv[i],"%x",&c) == 1, "invalid hex input %s", argv[i]);
        const utf8proc_property_t *p = utf8proc_get_property(c);

        if (utf8proc_codepoint_valid(c))
            cstr[utf8proc_encode_char(c, cstr)] = 0;
        else
            strcat((char*)cstr, "N/A");
        utf8proc_map(cstr, 0, &map, UTF8PROC_NULLTERM | UTF8PROC_CASEFOLD);

        printf("U+%s: %s\n"
            "  category = %s\n"
            "  combining_class = %d\n"
            "  bidi_class = %d\n"
            "  decomp_type = %d\n"
            "  uppercase_mapping = %x\n"
            "  lowercase_mapping = %x\n"
            "  titlecase_mapping = %x\n"
            "  casefold = %s\n"
            "  comb_index = %d\n"
            "  bidi_mirrored = %d\n"
            "  comp_exclusion = %d\n"
            "  ignorable = %d\n"
            "  control_boundary = %d\n"
            "  boundclass = %d\n"
            "  charwidth = %d\n",
        argv[i], (char*) cstr,
        utf8proc_category_string(c),
        p->combining_class,
        p->bidi_class,
        p->decomp_type,
        utf8proc_toupper(c),
        utf8proc_tolower(c),
        utf8proc_totitle(c),
        (char *) map,
        p->comb_index,
        p->bidi_mirrored,
        p->comp_exclusion,
        p->ignorable,
        p->control_boundary,
        p->boundclass,
        utf8proc_charwidth(c));
        free(map);
    }
    return 0;
}
