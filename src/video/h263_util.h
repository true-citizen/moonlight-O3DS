/*
 * Minimal H.263 / H.263+ picture header parser.
 *
 * H.263+ has no NAL units or parameter sets, so this is the only way to
 * sanity check a decode unit before it is handed to FFmpeg: it validates the
 * picture start code, tells intra from inter pictures and, when the picture
 * carries it, reports the picture size (standard source format or the H.263+
 * custom picture format).
 */
#ifndef H263_UTIL_H
#define H263_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    H263_OK = 0,
    H263_ERR_TRUNCATED,    // not enough data for a picture header
    H263_ERR_START_CODE,   // missing picture start code (PSC)
    H263_ERR_MARKER,       // fixed marker bits are wrong
    H263_ERR_FORMAT,       // reserved / unsupported source format
    H263_ERR_PICTURE_TYPE, // reserved picture type code
} H263Status;

typedef struct {
    bool is_intra;        // I (or EI) picture, i.e. can start a stream
    bool has_dimensions;  // width / height are valid for this picture
    unsigned int width;
    unsigned int height;
    bool plus;            // header used the H.263+ extended PTYPE (PLUSPTYPE)
} H263PictureInfo;

// Parses the picture header at the start of |data|. |info| is only valid when
// H263_OK is returned.
H263Status h263_parse_picture_header(const uint8_t *data, size_t length,
                                     H263PictureInfo *info);

const char *h263_status_str(H263Status status);

// H.263+ carries the stream size as the custom picture format, which the host
// encoder only fills for sizes that are a multiple of 4, up to 2048x1152.
bool h263_size_supported(int width, int height);

#ifdef __cplusplus
}
#endif

#endif
