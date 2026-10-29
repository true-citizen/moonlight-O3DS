/*
 * Minimal MPEG-2 video picture header parser.
 *
 * MPEG-2 has no parameter sets that travel separately, so this is the only way
 * to sanity check a decode unit before it is handed to FFmpeg: it validates the
 * start code, tells intra from inter pictures and, when the unit begins with a
 * sequence header (which the host puts in front of every key frame), reports
 * the stream size.
 *
 * The host sends each picture as one unit. A key frame unit starts with a
 * sequence header (0x000001B3) and any other unit starts with the picture
 * header (0x00000100).
 */
#ifndef MPEG2_UTIL_H
#define MPEG2_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MPEG2_OK = 0,
    MPEG2_ERR_TRUNCATED,    // not enough data for the headers
    MPEG2_ERR_START_CODE,   // missing 00 00 01 start code prefix
    MPEG2_ERR_FIRST_CODE,   // starts with neither a sequence nor a picture header
    MPEG2_ERR_MARKER,       // the sequence header marker bit is wrong
    MPEG2_ERR_PICTURE_TYPE, // reserved picture coding type
} Mpeg2Status;

typedef struct {
    bool is_intra;        // I picture, i.e. can start a stream
    bool has_dimensions;  // width / height are valid for this unit
    unsigned int width;
    unsigned int height;
} Mpeg2PictureInfo;

// Parses the headers at the start of |data|. |info| is only valid when
// MPEG2_OK is returned.
Mpeg2Status mpeg2_parse_picture_header(const uint8_t *data, size_t length,
                                       Mpeg2PictureInfo *info);

const char *mpeg2_status_str(Mpeg2Status status);

#ifdef __cplusplus
}
#endif

#endif
