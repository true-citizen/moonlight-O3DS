# H.263+ extension to the Moonlight protocol

This fork lets the Old 3DS ask the host for H.263+ (H.263 version 2) instead of
H.264, because the original 3DS cannot decode H.264 fast enough in software.
Stock Sunshine and GameStream do not know about it, so a host needs the
matching Sunshine change (<https://github.com/true-citizen/Sunshine-h263p>).
This client has no H.264 support, so if the host does not offer H.263+ the
stream fails to start.

## Values

| What | Value | Where |
| --- | --- | --- |
| Client format flag | `VIDEO_FORMAT_H263 = 0x0010`, mask `VIDEO_FORMAT_MASK_H263 = 0x00F0` | `moonlight-common-c` `Limelight.h` (patched) |
| Host capability bit | `SCM_H263 = 0x10000000` in `ServerCodecModeSupport` | `/serverinfo` response |
| RTSP DESCRIBE | `a=rtpmap:99 H263-1998/90000` | host SDP |
| ANNOUNCE | `x-nv-vqos[0].bitStreamFormat = 3`, `x-nv-clientSupportHevc = 0` | client SDP |

## Negotiation

1. The client sets `supportedVideoFormats = VIDEO_FORMAT_H263`
   when `video_codec = 1` is configured.
2. The host advertises `SCM_H263` in `/serverinfo` and the `H263-1998/90000`
   rtpmap in the RTSP DESCRIBE reply, but only when `h263_mode = 1` and the
   software encoder (`encoder = software`) can produce H.263+.
3. If the client asked for H.263+, the host advertised `SCM_H263` and the
   rtpmap is present, the client negotiates `VIDEO_FORMAT_H263`. In any other
   case the handshake fails (the client never negotiates H.264).
4. The client sends `bitStreamFormat = 3` and the host encodes with FFmpeg's
   `h263p` encoder.

## Bitstream

- Each frame is a complete H.263+ picture starting with the picture start code
  (PSC). There are no NAL units, so the client does not parse the stream and
  trusts the frame header for IDR detection, like AV1.
- The stream size must match the width and height configured on the client.
  H.263+ carries non-standard sizes with the custom picture format, so the host
  needs both sizes to be a multiple of 4 (maximum 2048x1152). The client
  prints a warning before streaming when the configured size does not fit.
- Any frame rate and bitrate works.
- The client checks each picture header (`src/video/h263_util.c`) and asks for
  a new IDR frame if one is damaged.
- Only a single reference frame is used, so reference frame invalidation is not
  supported with H.263+.
- Decoding is always in software (FFmpeg `h263`, then `h263p`). The hardware
  decoder setting is ignored.

## Building

`moonlight-common-c` is pinned to an upstream commit, so the client side of the
negotiation is carried as a patch:

```
./3ds/apply-patches.sh
```

The script can be run more than once; it skips patches that are already applied.
