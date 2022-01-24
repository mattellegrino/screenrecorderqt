#ifndef FFMPEGSCREENRECORDER_FFMPEGLIB_H
#define FFMPEGSCREENRECORDER_FFMPEGLIB_H


#pragma warning(disable:4819)

extern "C" {
    #define __STDC_CONSTANT_MACROS
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavformat/avio.h>
    #include <libavutil/avutil.h>
    #include <libavutil/file.h>
    #include <libavutil/time.h>
    #include <libavutil/audio_fifo.h>
    #include <libavutil/imgutils.h>
    #include <libswresample/swresample.h>
    #include <libswscale/swscale.h>
    #include <libavdevice/avdevice.h>
    #include <libavcodec/avfft.h>
    #include <libavfilter/avfilter.h>
    #include "libavfilter/buffersink.h"
    #include "libavfilter/buffersrc.h"
    #include "libavutil/opt.h"
    #include "libavutil/common.h"
    #include "libavutil/channel_layout.h"
    #include "libavutil/mathematics.h"
    #include "libavutil/samplefmt.h"
    #include "libavutil/pixdesc.h"
    #include <ctime>
}

#endif //FFMPEGSCREENRECORDER_FFMPEGLIB_H