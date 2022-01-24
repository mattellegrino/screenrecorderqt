#ifndef SCREEN_CAPTURE_FFMPEG_SCREENRECORDER_H
#define SCREEN_CAPTURE_FFMPEG_SCREENRECORDER_H

#include <string>
#include <atomic>
#include <thread>
#include <condition_variable>
#include "ffmpeglib.h"
#include "outputColors.h"
using namespace std;
const AVSampleFormat requireAudioFmt = AV_SAMPLE_FMT_FLTP;
class ScreenRecorder {
private:
    string              outFile;
    bool                audioRec;
    const char*         resolution;
    const char*         oX;
    const char*         oY;

    string              deviceName;
    string              failReason;
    AVFormatContext*    audioInFormatContext{};
    AVStream*           audioInStream{};
    AVCodecContext*     audioInCodecContext{};
    SwrContext*         audioConverter{};
    AVAudioFifo*        audioFifo{};
    AVFormatContext*    outFormatContext{};
    mutex               outFmtCtxLock;
    AVStream*           audioOutStream{};
    AVCodecContext*     audioOutCodecContext{};
    AVInputFormat*      videoInputFormat{};
    AVCodecContext*     videoInCodecContext{};
    AVFormatContext*    videoInFormatContext{};
    AVDictionary*       videoOptions{};
    AVCodecContext*     videoOutCodecContext{};
    mutex               audioInLk;
    mutex               videoInLk;
    AVStream*           videoInStream{};
    AVStream*           videoOutStream{};
    AVCodec*            videoInCodec{};
    AVCodec*            audioInCodec{};
    SwsContext*         videoConverter{};
    int                 videoCodecId{};
    int                 videoStreamIndex{};
    atomic_bool         running;
    atomic_bool         executing;
    thread*             audioThread{};
    thread*             videoThread{};



    void open();
    void initFile();
    void openVideo();
    void openAudio();
    void readFrames();
    void decodeEncodeVideo();
    void decodeEncodeAudio();
    void findVideoStream();
    void findAudioStream();
    void setVideoOutCC(AVCodec* codec);
    void setAudioOutCC(AVCodec* codec);
    void initAudioStream();
    void initVideoStream();
    int64_t incrementTs();
    AVInputFormat* cp_find_input_format();


public:
    explicit ScreenRecorder();
    ~ScreenRecorder();
    void start(const string& filename, bool audioRec, int oX, int oY, const string& resolution);
    void stop();
    void pause();
    void resume();
};


#endif //SCREEN_CAPTURE_FFMPEG_SCREENRECORDER_H
