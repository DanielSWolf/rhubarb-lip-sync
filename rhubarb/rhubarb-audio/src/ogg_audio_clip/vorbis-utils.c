#include <vorbis/vorbisfile.h>
#include <stdlib.h>
#include <stdio.h>

// Creates an OggVorbis_File structure on the heap so that the caller doesn't need to know its size
extern OggVorbis_File* vu_create_oggvorbisfile() {
    return malloc(sizeof (OggVorbis_File));
}

extern void vu_free_oggvorbisfile(OggVorbis_File* vf) {
    ov_clear(vf); // never fails
    free(vf);
}

extern const int vu_seek_origin_start = SEEK_SET;
extern const int vu_seek_origin_current = SEEK_CUR;
extern const int vu_seek_origin_end = SEEK_END;
