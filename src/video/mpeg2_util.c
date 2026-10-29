#include "mpeg2_util.h"

#include <string.h>

#define START_CODE_PICTURE 0x00
#define START_CODE_SEQUENCE_HEADER 0xB3
#define START_CODE_EXTENSION 0xB5

// A sequence header is the 4 byte start code followed by 8 fixed bytes. The
// quantiser matrices that may follow contain no zero bytes, so a start code can
// still be found by scanning.
#define SEQUENCE_HEADER_MIN_SIZE (4 + 8)

// How far into a unit the picture header is searched for. The sequence header,
// its extensions and the GOP header all come first and add up to far less.
#define SCAN_LIMIT 1024

static bool is_start_code(const uint8_t *data, size_t length, size_t pos,
                          uint8_t code) {
    return pos + 4 <= length && data[pos] == 0x00 && data[pos + 1] == 0x00 &&
           data[pos + 2] == 0x01 && data[pos + 3] == code;
}

// Finds the next start code with the given value at or after |from|, looking
// at no more than |limit| bytes. Returns (size_t)-1 when there is none.
static size_t find_start_code(const uint8_t *data, size_t limit, size_t from,
                              uint8_t code) {
    for (size_t pos = from; pos + 4 <= limit; pos++) {
        if (is_start_code(data, limit, pos, code)) {
            return pos;
        }
    }
    return (size_t)-1;
}

Mpeg2Status mpeg2_parse_picture_header(const uint8_t *data, size_t length,
                                       Mpeg2PictureInfo *info) {
    memset(info, 0, sizeof(*info));

    if (length < 4) {
        return MPEG2_ERR_TRUNCATED;
    }
    if (data[0] != 0x00 || data[1] != 0x00 || data[2] != 0x01) {
        return MPEG2_ERR_START_CODE;
    }

    const size_t limit = length < SCAN_LIMIT ? length : SCAN_LIMIT;
    size_t picture_search_from = 0;

    if (data[3] == START_CODE_SEQUENCE_HEADER) {
        if (length < SEQUENCE_HEADER_MIN_SIZE) {
            return MPEG2_ERR_TRUNCATED;
        }

        // horizontal_size (12), vertical_size (12), aspect (4), frame rate (4),
        // bit rate (18), marker (1), ...
        unsigned int width = ((unsigned int)data[4] << 4) | (data[5] >> 4);
        unsigned int height = ((unsigned int)(data[5] & 0x0F) << 8) | data[6];
        if (((data[10] >> 5) & 1) == 0) {
            return MPEG2_ERR_MARKER;
        }
        picture_search_from = SEQUENCE_HEADER_MIN_SIZE;

        // The sequence extension (id 1) carries the top 2 bits of each size,
        // which only matter for sizes of 4096 and up.
        size_t ext = find_start_code(data, limit, SEQUENCE_HEADER_MIN_SIZE,
                                     START_CODE_EXTENSION);
        if (ext != (size_t)-1 && ext + 7 <= length && (data[ext + 4] >> 4) == 1) {
            width |= (unsigned int)(((data[ext + 5] & 0x01) << 1) |
                                    (data[ext + 6] >> 7))
                     << 12;
            height |= (unsigned int)((data[ext + 6] >> 5) & 0x03) << 12;
            picture_search_from = ext + 4;
        }

        info->width = width;
        info->height = height;
        info->has_dimensions = width != 0 && height != 0;
    } else if (data[3] != START_CODE_PICTURE) {
        return MPEG2_ERR_FIRST_CODE;
    }

    // temporal_reference (10), picture_coding_type (3), ... follow the start code
    size_t picture = find_start_code(data, limit, picture_search_from,
                                     START_CODE_PICTURE);
    if (picture == (size_t)-1 || picture + 6 > length) {
        return MPEG2_ERR_TRUNCATED;
    }
    unsigned int picture_type = (data[picture + 5] >> 3) & 0x07;
    if (picture_type < 1 || picture_type > 3) { // 1 = I, 2 = P, 3 = B
        return MPEG2_ERR_PICTURE_TYPE;
    }
    info->is_intra = picture_type == 1;
    return MPEG2_OK;
}

const char *mpeg2_status_str(Mpeg2Status status) {
    switch (status) {
    case MPEG2_OK:               return "ok";
    case MPEG2_ERR_TRUNCATED:    return "truncated header";
    case MPEG2_ERR_START_CODE:   return "bad start code";
    case MPEG2_ERR_FIRST_CODE:   return "unexpected first header";
    case MPEG2_ERR_MARKER:       return "bad marker bit";
    case MPEG2_ERR_PICTURE_TYPE: return "reserved picture type";
    }
    return "unknown";
}
