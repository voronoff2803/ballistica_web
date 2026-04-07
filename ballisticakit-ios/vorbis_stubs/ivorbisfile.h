// Stub ivorbisfile.h for iOS build (Tremor integer-only Vorbis).
#ifndef IVORBISFILE_H_STUB
#define IVORBISFILE_H_STUB

#include <cstdio>
#include <cstdint>

typedef long ogg_int64_t;

typedef struct {
  long rate;
  int channels;
  int dummy[8];
} vorbis_info;

typedef struct { int dummy; } vorbis_comment;

typedef struct {
  void* datasource;
  int seekable;
  int dummy[16];
} OggVorbis_File;

typedef struct {
  size_t (*read_func)(void* ptr, size_t size, size_t nmemb, void* datasource);
  int (*seek_func)(void* datasource, ogg_int64_t offset, int whence);
  int (*close_func)(void* datasource);
  long (*tell_func)(void* datasource);
} ov_callbacks;

// Return 0 = success so the engine doesn't try fallback paths.
static inline int ov_open_callbacks(void* ds, OggVorbis_File* vf, const char*, long, ov_callbacks cb) {
  if (vf) { vf->datasource = ds; vf->seekable = 0; }
  // Close the file handle via the callback so it doesn't leak.
  if (cb.close_func && ds) { cb.close_func(ds); }
  return 0;
}
static inline int ov_clear(OggVorbis_File*) { return 0; }
// Return a valid vorbis_info so the engine doesn't crash on nullptr.
static vorbis_info _stub_vinfo = {44100, 2, {}};
static inline vorbis_info* ov_info(OggVorbis_File*, int) { return &_stub_vinfo; }
// Return a small chunk of silence on first read, then 0 (EOF).
// Without at least some data, the engine throws "zero-length buffer" errors.
static inline long ov_read(OggVorbis_File* vf, char* buf, int len, int*) {
  // Use seekable field as "already read" flag (it's unused in our stub).
  if (vf && !vf->seekable) {
    vf->seekable = 1;
    int n = len < 4096 ? len : 4096;
    for (int i = 0; i < n; i++) buf[i] = 0;
    return n;
  }
  return 0;
}
static inline ogg_int64_t ov_pcm_total(OggVorbis_File*, int) { return 0; }

#endif
