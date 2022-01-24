#include "../include/ScreenRecorder.h"
#include <iostream>
#include <cassert>

using namespace std;

ScreenRecorder::ScreenRecorder() {
    avdevice_register_all();
}

ScreenRecorder::~ScreenRecorder() {
    avformat_free_context(videoInFormatContext);
    avformat_free_context(audioInFormatContext);
    avformat_free_context(outFormatContext);
    avcodec_free_context(&videoInCodecContext);
    avcodec_free_context(&audioInCodecContext);
    avcodec_free_context(&audioOutCodecContext);
    avcodec_free_context(&videoOutCodecContext);
    av_audio_fifo_free(audioFifo);
    swr_free(&audioConverter);
    sws_freeContext(videoConverter);
}

void ScreenRecorder::stop() {
    cout << "entered stop" << endl;
    this->executing = false;
    audioInLk.lock();
    videoInLk.lock();
    avformat_close_input(&videoInFormatContext);
    avformat_close_input(&audioInFormatContext);
    audioInLk.unlock();
    videoInLk.unlock();
    videoThread->join();
    audioThread->join();
    outFmtCtxLock.lock();
    int ret = av_write_trailer(outFormatContext);
    outFmtCtxLock.unlock();
    if (ret < 0) throw std::runtime_error("Can not write file trailer.");
    avio_close(outFormatContext->pb);
    std::clog << "Finished recording." << std::endl;
    fflush(stdout);
}

void ScreenRecorder::pause() {
    this->running = false;
    cout << BOLDMAGENTA << "PAUSED" << RESET << endl;
}

void ScreenRecorder::resume() {
    this->running = true;
    cout << BOLDMAGENTA << "RESUMED" << RESET << endl;
}

void ScreenRecorder::start(const string& filename, bool audioRec, int oX, int oY, const string& resolution) {
    this->outFile = filename;
    this->resolution = resolution.c_str();
    this->oX = std::to_string(oX).c_str();
    this->oY = std::to_string(oY).c_str();
    this->audioRec = audioRec;
    try {
        this->open();
    }
    catch (std::exception& e) {
        this->failReason = e.what();
    }


    initFile();
    running = true;
    executing = true;

    videoThread = new std::thread([this]() {
        try {
            this->decodeEncodeVideo();
        }
        catch (std::exception& e) {
            this->failReason = e.what();
        }
    });
    audioThread = new std::thread([this]() {
        try {
            this->decodeEncodeAudio();
        }
        catch (std::exception& e) {
            this->failReason = e.what();
        }
    });
}

void ScreenRecorder::open() {
    openVideo();
    openAudio();
}

void ScreenRecorder::initFile() {
    if((avformat_alloc_output_context2(&outFormatContext, nullptr, nullptr, outFile.c_str())) < 0)
        throw std::runtime_error("Failed to alloc ouput context");

    if((avio_open(&outFormatContext->pb, outFile.c_str(), AVIO_FLAG_WRITE)) < 0)
        throw std::runtime_error("Fail to open output file.");

    initVideoStream();

    initAudioStream();

    if ((avformat_write_header(outFormatContext , &videoOptions)) < 0)
        throw std::runtime_error("Error writing header context.");
}
