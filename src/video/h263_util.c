#include "h263_util.h"

#include <string.h>

typedef struct {
    const uint8_t *data;
    size_t size_bits;
    size_t pos;
} BitReader;

static bool br_has(const BitReader *br, unsigned int n) {
    return br->pos + n <= br->size_bits;
}

// Caller must check br_has() first. n <= 32.
static uint32_t br_get(BitReader *br, unsigned int n) {
    uint32_t value = 0;
    for (unsigned int i = 0; i < n; i++) {
        size_t byte = br->pos >> 3;
        unsigned int bit = 7 - (br->pos & 7);
        value = (value << 1) | ((br->data[byte] >> bit) & 1);
        br->pos++;
    }
    return value;
}

#define NEED(br, n)                                                            \
    do {                                                                       \
        if (!br_has((br), (n)))                                                \
            return H263_ERR_TRUNCATED;                                         \
    } while (0)

static bool standard_size(unsigned int source_format, unsigned int *w,
                          unsigned int *h) {
    switch (source_format) {
    case 1: *w = 128;  *h = 96;   return true; // sub-QCIF
    case 2: *w = 176;  *h = 144;  return true; // QCIF
    case 3: *w = 352;  *h = 288;  return true; // CIF
    case 4: *w = 704;  *h = 576;  return true; // 4CIF
    case 5: *w = 1408; *h = 1152; return true; // 16CIF
    default: return false;
    }
}

H263Status h263_parse_picture_header(const uint8_t *data, size_t length,
                                     H263PictureInfo *info) {
    BitReader br = {data, length * 8, 0};
    memset(info, 0, sizeof(*info));

    // PSC (22 bits) + TR (8 bits) + PTYPE bits 1-8
    NEED(&br, 22 + 8 + 8);
    if (br_get(&br, 22) != 0x20) {
        return H263_ERR_START_CODE;
    }
    br_get(&br, 8); // TR

    if (br_get(&br, 2) != 2) { // "10"
        return H263_ERR_MARKER;
    }
    br_get(&br, 3); // split screen, document camera, freeze picture release
    unsigned int source_format = br_get(&br, 3);

    if (source_format != 7) {
        // Baseline H.263
        if (!standard_size(source_format, &info->width, &info->height)) {
            return H263_ERR_FORMAT;
        }
        info->has_dimensions = true;
        NEED(&br, 1);
        info->is_intra = br_get(&br, 1) == 0;
        return H263_OK;
    }

    // H.263+: PLUSPTYPE
    info->plus = true;
    NEED(&br, 3);
    unsigned int ufep = br_get(&br, 3);
    if (ufep != 0 && ufep != 1) {
        return H263_ERR_MARKER;
    }

    unsigned int opptype_format = 0;
    if (ufep == 1) {
        NEED(&br, 18);
        opptype_format = br_get(&br, 3);
        br_get(&br, 11); // custom PCF, UMV, SAC, AP, AIC, DF, SS, RPS, ISD, AIV, MQ
        if (br_get(&br, 1) != 1) { // bit 15 is always 1
            return H263_ERR_MARKER;
        }
        if (br_get(&br, 3) != 0) { // bits 16-18 are always 0
            return H263_ERR_MARKER;
        }
    }

    // MPPTYPE (9 bits)
    NEED(&br, 9);
    unsigned int picture_type = br_get(&br, 3);
    br_get(&br, 3); // RPR, RRU, rounding type
    if (br_get(&br, 3) != 1) { // bits 7-9 are "001"
        return H263_ERR_MARKER;
    }
    if (picture_type > 5) {
        return H263_ERR_PICTURE_TYPE;
    }
    info->is_intra = picture_type == 0; // I picture

    // CPM, PSBI
    NEED(&br, 1);
    if (br_get(&br, 1)) {
        NEED(&br, 2);
        br_get(&br, 2);
    }

    if (ufep == 1) {
        if (opptype_format == 6) {
            // Custom picture format: PAR(4) PWI(9) '1' PHI(9) [EPAR(16)]
            NEED(&br, 4 + 9 + 1 + 9);
            unsigned int par = br_get(&br, 4);
            unsigned int pwi = br_get(&br, 9);
            if (br_get(&br, 1) != 1) {
                return H263_ERR_MARKER;
            }
            unsigned int phi = br_get(&br, 9);
            if (par == 15) {
                NEED(&br, 16);
            }
            info->width = (pwi + 1) * 4;
            info->height = phi * 4;
            info->has_dimensions = info->width != 0 && info->height != 0;
        } else if (standard_size(opptype_format, &info->width, &info->height)) {
            info->has_dimensions = true;
        } else {
            return H263_ERR_FORMAT;
        }
    }
    // With UFEP = 0 the format of an earlier picture still applies, so this
    // picture has no dimensions of its own.
    return H263_OK;
}

const char *h263_status_str(H263Status status) {
    switch (status) {
    case H263_OK:               return "ok";
    case H263_ERR_TRUNCATED:    return "truncated header";
    case H263_ERR_START_CODE:   return "bad picture start code";
    case H263_ERR_MARKER:       return "bad marker bits";
    case H263_ERR_FORMAT:       return "unsupported source format";
    case H263_ERR_PICTURE_TYPE: return "reserved picture type";
    }
    return "unknown";
}
