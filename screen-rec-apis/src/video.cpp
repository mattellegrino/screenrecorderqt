#include "../include/ScreenRecorder.h"
#include <iostream>
#include <cassert>
#include "qglobal.h"
#include "windows.h"

std::wstring s2ws(const std::string& str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


void ScreenRecorder::openVideo() {
    videoOptions = nullptr;
    videoInFormatContext = nullptr;
    videoInFormatContext = avformat_alloc_context();

    if ((av_dict_set(&videoOptions, "framerate", "30", 0 )) < 0)
        throw std::runtime_error("Error in setting framerate value.");

    if ((av_dict_set(&videoOptions, "preset", "medium", 0 )) < 0)
        throw std::runtime_error("Error in setting preset value.");

    if ((av_dict_set(&videoOptions, "vsync", "vfr", 0 )) < 0)
        throw std::runtime_error("Error in setting preset value.");

    if ((av_dict_set(&videoOptions, "filter", "crop=80:60:200:100", 0 )) < 0)
        throw std::runtime_error("Error in setting crop value.");


    #ifdef Q_OS_LINUX
    videoInputFormat = av_find_input_format("x11grab");
    if ((avformat_open_input(&videoInFormatContext, ":1.0", videoInputFormat, &videoOptions)) != 0)
        throw std::runtime_error("Error while opening input device.");
    #endif

    #ifdef Q_OS_WIN32
    videoInputFormat = av_find_input_format("dshow");
    if ((avformat_open_input(&videoInFormatContext, "video=screen-capture-recorder", videoInputFormat, &videoOptions)) != 0)
        throw std::runtime_error("Error while opening input device.");
    #endif


    /*static DWORD newvalue = 300;
    std::string startx= {"300"};
    std::string valuename = {"start_x"};
    const std::wstring& valueName=s2ws(valuename);
    const std::wstring& data=s2ws(startx);
    HKEY hk;
    DWORD dw;
    DWORD dwType = REG_DWORD;
    char buf[255] = {0};
    DWORD dwBufSize = sizeof(newvalue);
    long n = RegOpenKeyEx(HKEY_CURRENT_USER,TEXT("Software\\screen-capture-recorder"),0,KEY_READ,&hk);
    if(n==ERROR_SUCCESS)
    {

    cout << "riuscito" << n << endl;
    }
    n = RegQueryValueEx (hk,TEXT("start_y"),NULL,NULL,(LPBYTE)&newvalue, &dwBufSize);
    if(n==ERROR_SUCCESS)
    {

    cout << "riuscito2" << n << endl;
    }
    n= RegSetValueEx(hk,NULL,0,REG_DWORD,(BYTE*)&newvalue,dwBufSize);
    if(n==ERROR_SUCCESS)
    {

    cout << "riuscito3" << endl;
    }else
    {

    cout << n << endl;
    }

    RegCloseKey(hk);*/


av_dict_set(&videoOptions, "video_size", this->resolution, 0);
av_dict_set(&videoOptions, "grab_x", this->oX, 0);
av_dict_set(&videoOptions, "grab_y", this->oY, 0);
av_dict_set(&videoOptions, "start_x", "20", 0);
av_dict_set(&videoOptions, "start_y", "30", 0);



    findVideoStream();

    videoInCodec = avcodec_find_decoder(videoInStream->codecpar->codec_id);
    if(videoInCodec == nullptr)
        throw std::runtime_error("Couldn't find decoder.");

    if ((videoInCodecContext = avcodec_alloc_context3(videoInCodec)) == nullptr)
        throw std::runtime_error("Couldn't allocate context.");

    if ((avcodec_parameters_to_context(videoInCodecContext, videoInStream->codecpar)) < 0)
        throw std::runtime_error("Error while passing parameters to context.");

    if ((avcodec_open2(videoInCodecContext, videoInCodec,nullptr)) < 0)
        throw std::runtime_error("Unable to open the av codec.");
}

void ScreenRecorder::decodeEncodeVideo() {
    AVFrame* inputFrame = av_frame_alloc();
    AVFrame* outputFrame = av_frame_alloc();
    AVPacket* inputPacket = av_packet_alloc();
    AVPacket* outputPacket = av_packet_alloc();
    int64_t ts = 0;
    int frameCount = 0;
    bool validRead = false;

    outputFrame->format = videoOutCodecContext->pix_fmt;
    outputFrame->width = videoOutCodecContext->width;
    outputFrame->height = videoOutCodecContext-> height;

    int nbytes = av_image_get_buffer_size(
            videoOutCodecContext->pix_fmt,
            videoOutCodecContext->width,
            videoOutCodecContext->height,
            32);

    auto *videoOutbuf = (uint8_t*) av_malloc(nbytes);

    if (videoOutbuf == nullptr)
        throw std::runtime_error("Couldn't allocate memory.");

    if ((av_image_fill_arrays(
            outputFrame->data,
            outputFrame->linesize,
            videoOutbuf,
            AV_PIX_FMT_YUV420P,
            videoOutCodecContext->width,
            videoOutCodecContext->height,
            1
    )) < 0) throw std::runtime_error("Error occurred while filling image array.");

    videoConverter = sws_getContext(
            videoInCodecContext->width,
            videoInCodecContext->height,
            videoInCodecContext->pix_fmt,
            videoOutCodecContext->width,
            videoOutCodecContext->height,
            videoOutCodecContext->pix_fmt,
            SWS_BICUBIC,
            nullptr,
            nullptr,
            nullptr
    );

    while (executing) {

        videoInLk.lock();
        if (videoInFormatContext == nullptr) {
            videoInLk.unlock();
            break;
        }

        validRead = running;
        if (av_read_frame(videoInFormatContext, inputPacket) < 0) {
            videoInLk.unlock();
            break;
        }
        videoInLk.unlock();

        if (validRead) {

            if ((avcodec_send_packet(videoInCodecContext, inputPacket)) < 0)
                throw std::runtime_error("Can not send pkt in decoding.");

            if ((avcodec_receive_frame(videoInCodecContext, inputFrame)) < 0)
                throw std::runtime_error("Can not receive frame in decoding.");

            sws_scale(videoConverter, inputFrame->data, inputFrame->linesize, 0, videoInCodecContext->height,
                      outputFrame->data, outputFrame->linesize);
            outputPacket->data = nullptr;
            outputPacket->size = 0;

            if ((avcodec_send_frame(videoOutCodecContext, outputFrame)) < 0)
                throw std::runtime_error("Fail to send frame in encoding.");

            int value = avcodec_receive_packet(videoOutCodecContext, outputPacket);
            if (value == AVERROR(EAGAIN))
                continue;

            ts += incrementTs();
            outputPacket->pts = ts;
            outputPacket->dts = ts;

            outFmtCtxLock.lock();
            if (av_write_frame(outFormatContext, outputPacket) != 0)
                throw std::runtime_error("Error writing frame");
            outFmtCtxLock.unlock();
            cout << BLUE << "Written frame " << frameCount++ << " (size = " << outputPacket->size << ")" << RESET
                 << endl;

            av_packet_unref(outputPacket);
        }
        av_packet_unref(inputPacket);
    }
    av_free(videoOutbuf);
    av_frame_free(&inputFrame);
    av_frame_free(&outputFrame);
    av_packet_free(&inputPacket);
    av_packet_free(&outputPacket);
}

void ScreenRecorder::findVideoStream() {
    videoStreamIndex = -1;

    for (int i = 0; i < videoInFormatContext->nb_streams; i++) {
        if (videoInFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            videoInStream = videoInFormatContext->streams[i];
            break;
        }
    }
    if (videoStreamIndex == -1)
        throw std::runtime_error("Couldn't find video stream index.");
}

void ScreenRecorder::setVideoOutCC(AVCodec *codec) {
    if (!(videoOutCodecContext = avcodec_alloc_context3(codec)))
        throw std::runtime_error("Couldn't allocate codec context.");
    videoOutCodecContext = videoOutStream->codec;
    //avcodec_parameters_to_context(videoOutCodecContext, videoOutStream->codecpar);
    videoOutCodecContext->codec_id = AV_CODEC_ID_MPEG4;// AV_CODEC_ID_MPEG4; // AV_CODEC_ID_H264 // AV_CODEC_ID_MPEG1VIDEO
    videoOutCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    videoOutCodecContext->pix_fmt  = AV_PIX_FMT_YUV420P;
    videoOutCodecContext->bit_rate = 10000000;
    videoOutCodecContext->width = 1920;
    videoOutCodecContext->height = 1080;
    videoOutCodecContext->gop_size = 0;
    videoOutCodecContext->max_b_frames = 0;
    videoOutCodecContext->time_base.num = 1;
    videoOutCodecContext->time_base.den = 30;
    if (videoCodecId == AV_CODEC_ID_H264)
        av_opt_set(videoOutCodecContext->priv_data, "preset", "slow", 0);
    outFmtCtxLock.lock();
    if (outFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
        videoOutCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    outFmtCtxLock.unlock();
}

void ScreenRecorder::initVideoStream() {
    if (!(videoOutStream = avformat_new_stream(outFormatContext , nullptr)))
        throw std::runtime_error("Couldn't create new stream.");

    AVCodec* videoOutCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    if(!videoOutCodec)
        throw std::runtime_error("Couldn't find the encoder.");

    setVideoOutCC(videoOutCodec);

    if ((avcodec_open2(videoOutCodecContext, videoOutCodec, nullptr)) < 0)
        throw std::runtime_error("Couldn't open av codec.");
}

int64_t ScreenRecorder::incrementTs() {
    return av_rescale_q(1, videoOutCodecContext->time_base, videoOutStream->time_base);
}
