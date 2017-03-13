#include <stdio.h>
#include <stdint.h>

#define AV_LOG_PANIC     0

/**
 * Something went wrong and recovery is not possible.
 * For example, no header was found for a format which depends
 * on headers or an illegal combination of parameters is used.
 */
#define AV_LOG_FATAL     8

/**
 * Something went wrong and cannot losslessly be recovered.
 * However, not all future data is affected.
 */
#define AV_LOG_ERROR    16

/**
 * Something somehow does not look correct. This may or may not
 * lead to problems. An example would be the use of '-vstrict -2'.
 */
#define AV_LOG_WARNING  24

/**
 * Standard information.
 */
#define AV_LOG_INFO     32

/**
 * Detailed information.
 */
#define AV_LOG_VERBOSE  40

/**
 * Stuff which is only useful for libav* developers.
 */
#define AV_LOG_DEBUG    48

/**
 * Extremely verbose debugging, useful for libav* development.
 */
#define AV_LOG_TRACE    56

typedef enum {
    AV_CLASS_CATEGORY_NA = 0,
    AV_CLASS_CATEGORY_INPUT,
    AV_CLASS_CATEGORY_OUTPUT,
    AV_CLASS_CATEGORY_MUXER,
    AV_CLASS_CATEGORY_DEMUXER,
    AV_CLASS_CATEGORY_ENCODER,
    AV_CLASS_CATEGORY_DECODER,
    AV_CLASS_CATEGORY_FILTER,
    AV_CLASS_CATEGORY_BITSTREAM_FILTER,
    AV_CLASS_CATEGORY_SWSCALER,
    AV_CLASS_CATEGORY_SWRESAMPLER,
    AV_CLASS_CATEGORY_DEVICE_VIDEO_OUTPUT = 40,
    AV_CLASS_CATEGORY_DEVICE_VIDEO_INPUT,
    AV_CLASS_CATEGORY_DEVICE_AUDIO_OUTPUT,
    AV_CLASS_CATEGORY_DEVICE_AUDIO_INPUT,
    AV_CLASS_CATEGORY_DEVICE_OUTPUT,
    AV_CLASS_CATEGORY_DEVICE_INPUT,
    AV_CLASS_CATEGORY_NB, ///< not part of ABI/API
}AVClassCategory;

static const uint32_t color[16 + AV_CLASS_CATEGORY_NB] =
{
    [AV_LOG_PANIC  /8] =  52 << 16 | 196 << 8 | 0x41,
    [AV_LOG_FATAL  /8] = 208 <<  8 | 0x41,
    [AV_LOG_ERROR  /8] = 196 <<  8 | 0x11,
    [AV_LOG_WARNING/8] = 226 <<  8 | 0x03,
    [AV_LOG_INFO   /8] = 253 <<  8 | 0x09,
    [AV_LOG_VERBOSE/8] =  40 <<  8 | 0x02,
    [AV_LOG_DEBUG  /8] =  34 <<  8 | 0x02,
    [AV_LOG_TRACE  /8] =  34 <<  8 | 0x07,
    // [16+AV_CLASS_CATEGORY_NA              ] = 34 << 8 | 0x09,
    // [16+AV_CLASS_CATEGORY_INPUT           ] = 219 << 8 | 0x15,
    // [16+AV_CLASS_CATEGORY_OUTPUT          ] = 201 << 8 | 0x05,
    // [16+AV_CLASS_CATEGORY_MUXER           ] = 213 << 8 | 0x15,
    // [16+AV_CLASS_CATEGORY_DEMUXER         ] = 207 << 8 | 0x05,
    // [16+AV_CLASS_CATEGORY_ENCODER         ] =  51 << 8 | 0x16,
    // [16+AV_CLASS_CATEGORY_DECODER         ] =  39 << 8 | 0x06,
    // [16+AV_CLASS_CATEGORY_FILTER          ] = 155 << 8 | 0x12,
    // [16+AV_CLASS_CATEGORY_BITSTREAM_FILTER] = 192 << 8 | 0x14,
    // [16+AV_CLASS_CATEGORY_SWSCALER        ] = 153 << 8 | 0x14,
    // [16+AV_CLASS_CATEGORY_SWRESAMPLER     ] = 147 << 8 | 0x14,
    // [16+AV_CLASS_CATEGORY_DEVICE_VIDEO_OUTPUT ] = 213 << 8 | 0x15,
    // [16+AV_CLASS_CATEGORY_DEVICE_VIDEO_INPUT  ] = 207 << 8 | 0x05,
    // [16+AV_CLASS_CATEGORY_DEVICE_AUDIO_OUTPUT ] = 213 << 8 | 0x15,
    // [16+AV_CLASS_CATEGORY_DEVICE_AUDIO_INPUT  ] = 207 << 8 | 0x05,
    // [16+AV_CLASS_CATEGORY_DEVICE_OUTPUT       ] = 213 << 8 | 0x15,
    // [16+AV_CLASS_CATEGORY_DEVICE_INPUT        ] = 207 << 8 | 0x05,
};

int main(int argc, char const *argv[])
{
    

    return 0;
}