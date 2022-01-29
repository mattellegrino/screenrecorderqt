#include "../include/ScreenRecorder.h"
#include <iostream>
#include <cassert>

void ScreenRecorder::openAudio() {
    AVDictionary* options = nullptr;

    AVInputFormat* audioInputFormat = cp_find_input_format();

    if (avformat_open_input(&audioInFormatContext, deviceName.c_str(), audioInputFormat, &options) != 0)
        throw std::runtime_error("Couldn't open input audio stream.");

    if (avformat_find_stream_info(audioInFormatContext, nullptr) < 0)
        throw std::runtime_error("Couldn't find audio stream information.");

    findAudioStream();

    audioInCodec = avcodec_find_decoder(audioInStream->codecpar->codec_id);
    if (audioInCodec == nullptr)
        throw std::runtime_error("Couldn't find decoder.");

    if ((audioInCodecContext = avcodec_alloc_context3(audioInCodec)) == nullptr)
        throw std::runtime_error("Couldn't allocate context.");

    if ((avcodec_parameters_to_context(audioInCodecContext, audioInStream->codecpar)) < 0)
        throw std::runtime_error("Error while passing parameters to context.");

    if (avcodec_open2(audioInCodecContext, audioInCodec, nullptr) < 0)
        throw std::runtime_error("Could not open video codec.");

    audioConverter = swr_alloc_set_opts (
            nullptr,
            av_get_default_channel_layout(audioInCodecContext->channels),
            requireAudioFmt,  // aac encoder only receive this format
            audioInCodecContext->sample_rate,
            av_get_default_channel_layout(audioInCodecContext->channels),
            static_cast<AVSampleFormat>(audioInStream->codecpar->format),
            audioInStream->codecpar->sample_rate,
            0,
            nullptr
    );
    swr_init(audioConverter);

    audioFifo = av_audio_fifo_alloc(requireAudioFmt, audioInCodecContext->channels, audioInCodecContext->sample_rate * 2);
}

void ScreenRecorder::decodeEncodeAudio() {
    AVFrame *inputFrame = av_frame_alloc();
    AVPacket *inputPacket = av_packet_alloc();
    AVPacket *outputPacket = av_packet_alloc();
    uint64_t frameCount = 0;
    int64_t ts = 0;
    bool validRead = false;

    int ret;

    while (executing) {

        audioInLk.lock();
        if(audioInFormatContext == nullptr) {
            audioInLk.unlock();
            break;
        }
        validRead = running;
        if (av_read_frame(audioInFormatContext, inputPacket) < 0) {
            audioInLk.unlock();
            break;
        }

        audioInLk.unlock();

        if (validRead) {

            if ((avcodec_send_packet(audioInCodecContext, inputPacket)) < 0)
                throw std::runtime_error("Can not send pkt in decoding.");

            if ((avcodec_receive_frame(audioInCodecContext, inputFrame)) < 0)
                throw std::runtime_error("Can not receive frame in decoding.");

            uint8_t** cSamples = nullptr;
            if ((av_samples_alloc_array_and_samples(&cSamples, nullptr, audioOutCodecContext->channels, inputFrame->nb_samples, requireAudioFmt, 0)) < 0)
                throw std::runtime_error("Fail to alloc samples by av_samples_alloc_array_and_samples.");

            if ((swr_convert(audioConverter, cSamples, inputFrame->nb_samples, (const uint8_t**)inputFrame->extended_data, inputFrame->nb_samples)) < 0)
                throw std::runtime_error("Fail to swr_convert.");

            if (av_audio_fifo_space(audioFifo) < inputFrame->nb_samples)
                throw std::runtime_error("audio buffer is too small.");

            if ((av_audio_fifo_write(audioFifo, reinterpret_cast<void**>(cSamples), inputFrame->nb_samples)) < 0)
                throw std::runtime_error("Fail to write fifo.");

            av_freep(&cSamples[0]);
            av_frame_unref(inputFrame);
            av_packet_unref(inputPacket);

            while (av_audio_fifo_size(audioFifo) >= audioOutCodecContext->frame_size) {
                AVFrame *outputFrame = av_frame_alloc();
                outputFrame->nb_samples = audioOutCodecContext->frame_size;
                outputFrame->channels = audioInCodecContext->channels;
                outputFrame->channel_layout = av_get_default_channel_layout(audioInCodecContext->channels);
                outputFrame->format = requireAudioFmt;
                outputFrame->sample_rate = audioOutCodecContext->sample_rate;

                ret = av_frame_get_buffer(outputFrame, 0);
                assert(ret >= 0);
                ret = av_audio_fifo_read(audioFifo, (void **) outputFrame->data, audioOutCodecContext->frame_size);
                assert(ret >= 0);

                //outputFrame->pts = static_cast<long>(frameCount) * audioOutStream->time_base.den * 1024 / audioOutCodecContext->sample_rate;

                if ((avcodec_send_frame(audioOutCodecContext, outputFrame)) < 0)
                    throw std::runtime_error("Fail to send frame in encoding.");

                av_frame_free(&outputFrame);
                ret = avcodec_receive_packet(audioOutCodecContext, outputPacket);
                if (ret == AVERROR(EAGAIN))
                    continue;
                else if (ret < 0)
                    throw std::runtime_error("Fail to receive packet in encoding.");

                outputPacket->stream_index = audioOutStream->index;
                outputPacket->duration = audioOutStream->time_base.den * 1024 / audioOutCodecContext->sample_rate;
                ts = static_cast<long>(frameCount) * audioOutStream->time_base.den * 1024 /
                     audioOutCodecContext->sample_rate;
                outputPacket->dts = outputPacket->pts = ts;

                outFmtCtxLock.lock();
                if ((av_write_frame(outFormatContext, outputPacket)) != 0)
                    throw std::runtime_error("Couldn't write frame.");
                outFmtCtxLock.unlock();
                cout << RED << "Written frame " << frameCount++ << " (size = " << outputPacket->size << ")" << RESET
                     << endl;
                av_packet_unref(outputPacket);
            }
            //if (cSamples)
                //av_freep(&cSamples[0]);
            //av_freep(&cSamples);
        }
    }
    av_packet_free(&inputPacket);
    av_packet_free(&outputPacket);
    av_frame_free(&inputFrame);
}

void ScreenRecorder::findAudioStream() {
    for (int i = 0; i < audioInFormatContext->nb_streams; i++) {
        if (audioInFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioInStream = audioInFormatContext->streams[i];
            break;
        }
    }
    if (!audioInStream) {
        throw std::runtime_error("Couldn't find a audio stream.");
    }
}

void ScreenRecorder::setAudioOutCC(AVCodec *codec) {
    audioOutCodecContext = avcodec_alloc_context3(codec);
    audioOutCodecContext->channels = audioInStream->codecpar->channels;
    audioOutCodecContext->channel_layout = av_get_default_channel_layout(audioInStream->codecpar->channels);
    //audioOutCodecContext->sample_rate = audioInStream->codecpar->sample_rate;
    audioOutCodecContext->sample_rate = select_sample_rate(codec);
    audioOutCodecContext->sample_fmt = codec->sample_fmts[0];  //for aac , there is AV_SAMPLE_FMT_FLTP =8
    audioOutCodecContext->bit_rate = 48000; //32000
    audioOutCodecContext->time_base.num = 1;
    audioOutCodecContext->time_base.den = audioOutCodecContext->sample_rate;

    outFmtCtxLock.lock();
    if (outFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
        audioOutCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    outFmtCtxLock.unlock();
}

void ScreenRecorder::initAudioStream() {
    AVCodec* audioOutCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (audioOutCodec == nullptr)
        throw std::runtime_error("Fail to find aac encoder. Please check your DLL.");

    setAudioOutCC(audioOutCodec);

    if (avcodec_open2(audioOutCodecContext, audioOutCodec, nullptr) < 0)
        throw std::runtime_error("Fail to open ouput audio encoder.");

    if((audioOutStream = avformat_new_stream(outFormatContext, audioOutCodec)) == nullptr)
        throw std::runtime_error("Fail to new a audio stream.");

    avcodec_parameters_from_context(audioOutStream->codecpar, audioOutCodecContext);
}

int ScreenRecorder::select_sample_rate(const AVCodec *codec) {
     const int *p;
     int best_samplerate = 0;

     if (!codec->supported_samplerates)
         return 44100;

     p = codec->supported_samplerates;
     while (*p) {
         if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
             best_samplerate = *p;
         p++;
     }
     return best_samplerate;
 }

AVInputFormat *ScreenRecorder::cp_find_input_format() {
#ifdef WINDOWS
    if (deviceName == "") {
        deviceName = DS_GetDefaultDevice("a");
        if (deviceName == "") {
            throw std::runtime_error("Fail to get default audio device, maybe no microphone.");
        }
    }
    deviceName = "audio=" + deviceName;
    return av_find_input_format("dshow");
#elif MACOS
    if(deviceName == "") deviceName = ":0";
    return av_find_input_format("avfoundation");
    //"[[VIDEO]:[AUDIO]]"
#elif UNIX
    if(deviceName.empty()) deviceName = "default";
    return av_find_input_format("alsa");
#elif __linux__
    if(deviceName.empty()) deviceName = "default";
    return av_find_input_format("alsa");
#endif
}
