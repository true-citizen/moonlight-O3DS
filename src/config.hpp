/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015-2017 Iwan Timmer
 *
 * Moonlight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Moonlight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Moonlight; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <Limelight.h>

#include <stdbool.h>

#define MAX_INPUTS 6

// The 3DS hardware (MVD) decoder only handles H.264, which this client no
// longer supports, so every codec is decoded in software. The numbers are kept
// so that old config files still mean the same thing (0 is treated as software).
enum VIDEO_DECODER_TYPE {
    LEGACY_HARDWARE_VIDEO_DECODER = 0,
    SOFTWARE_VIDEO_DECODER,
    DISABLE_VIDEO_DECODER
};

// Which codec to ask the host for. There is no H.264 and no fallback to it: the
// host has to be able to encode the chosen codec or the stream does not start.
// (0 was H.264 and is no longer valid, the numbers of the others are unchanged.)
enum VIDEO_CODEC_TYPE {
    H263_VIDEO_CODEC = 1,
    MPEG2_VIDEO_CODEC = 2
};

typedef struct _CONFIGURATION {
    STREAM_CONFIGURATION stream;
    char *app;
    char *action;
    char *address;
    char *mapping;
    char *platform;
    char *audio_device;
    char *config_file;
    char key_dir[4096];
    bool sops;
    bool localaudio;
    bool unsupported;
    bool quitappafter;
    bool viewonly;
    bool mouse_emulation;
    char *inputs[MAX_INPUTS];
    int inputsCount;
    bool hdr;
    unsigned short port;
    VIDEO_DECODER_TYPE video_decoder;
    VIDEO_CODEC_TYPE video_codec;
    bool motion_controls;
    bool swap_face_buttons;
    bool swap_triggers_and_shoulders;
    bool use_triggers_for_mouse;
} CONFIGURATION, *PCONFIGURATION;

bool config_file_parse(char *filename, PCONFIGURATION config);
void config_parse(int argc, char *argv[], PCONFIGURATION config);
void parse_argument(int c, char *value, PCONFIGURATION config);
void config_save(char *filename, PCONFIGURATION config);
