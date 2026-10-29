# MPEG-2 extension to the Moonlight protocol

MPEG-2 is a second alternative to H.264 for the original 3DS. It is cheaper to
decode in software than H.264 and, unlike H.263+, accepts any size the host
encoder supports. It uses the same decoder setup as the
[H.263+ extension](H263_PROTOCOL.md): the host needs the matching Sunshine
change (<https://github.com/true-citizen/Sunshine-h263p>) and the client falls
back to H.264 when the host cannot do MPEG-2.

## Values

| What | Value | Where |
| --- | --- | --- |
| Client format flag | `VIDEO_FORMAT_MPEG2 = 0x10000`, mask `VIDEO_FORMAT_MASK_MPEG2 = 0xF0000` | `moonlight-common-c` `Limelight.h` (patched) |
| Host capability bit | `SCM_MPEG2 = 0x20000000` in `ServerCodecModeSupport` | `/serverinfo` response |
| RTSP DESCRIBE | `a=rtpmap:100 MPV/90000` | host SDP |
| ANNOUNCE | `x-nv-vqos[0].bitStreamFormat = 4`, `x-nv-clientSupportHevc = 0` | client SDP |

The low 16 bits of the format flags are all used by upstream, hence bit 16.

## Negotiation

1. The client sets `supportedVideoFormats = VIDEO_FORMAT_MPEG2 | VIDEO_FORMAT_H264`
   when `video_codec = 2` is configured.
2. The host advertises `SCM_MPEG2` and the `MPV/90000` rtpmap only when
   `mpeg2_mode = 1` and the software encoder (`encoder = software`) can produce
   MPEG-2.
3. If all of that matches the client negotiates `VIDEO_FORMAT_MPEG2`, otherwise
   H.264.
4. The client sends `bitStreamFormat = 4` and the host encodes with FFmpeg's
   `mpeg2video` encoder.

## Bitstream

- Each frame is one picture. A key frame starts with a sequence header
  (`00 00 01 B3`), any other frame with the picture header (`00 00 01 00`).
- The host uses closed GOPs and no B frames. Limits of the encoder: width and
  height 2..16383 (`size % 4096 >= 2`), and the frame rate must be one of the
  MPEG-2 rates (24, 25, 30, 50, 60 and the 1000/1001 variants, times (n+1)/(d+1);
  144 fps is not possible).
- The stream size must match the configured width and height. The client checks
  each picture header (`src/video/mpeg2_util.c`) and asks for a new IDR frame
  when one is damaged.
- Decoding is always in software (FFmpeg `mpeg2video`). The hardware decoder
  setting is ignored.

## Performance

On the original 3DS 30 fps is the realistic target (see the benchmarks in
<https://github.com/Core-2-Extreme/Video_player_for_3DS>); New 3DS can do more.

## Building

The client side of the negotiation is carried as
`3ds/patches/moonlight-common-c-mpeg2.patch`, applied after the H.263+ patch by
`./3ds/apply-patches.sh` (safe to run repeatedly). FFmpeg is built with
`--enable-decoder=mpeg2video` (`3ds/build-ffmpeg.sh`).
