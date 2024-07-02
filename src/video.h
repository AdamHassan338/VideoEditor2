#ifndef VIDEO_H
#define VIDEO_H
#include <stdio.h>
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include<libavutil/avutil.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
#include <QDebug>
#include <unordered_map>
#include <QAudioBuffer>
#include <QAudioSink>
#include <QApplication>


struct VideoFrame{
    uint8_t* frameData;
    int linesize;
    int width;
    int height;
    /* TO DO */
    //store pixelFormat
    /* For now assume rgba32 */

    VideoFrame(){
        frameData = NULL;
        linesize = -1;
        width = -1;
        height = -1;
    }

    void clean(){
        if(frameData)
            delete[] frameData;
    }
};

struct AudioFrame{
    uint8_t* frameData;
    int frameSize;
    int64_t pts;
    AudioFrame(){
        frameData = NULL;
        frameSize = -1;
        pts=-1;
    }
    AudioFrame(uint8_t* data,int size) : frameData(data), frameSize(size){

    }

    void clean(){
        if(frameData)
            delete[] frameData;
    }
};

struct AudioBuffer{
    int64_t startPTS;
    int64_t endPTS;
    uint8_t* data;
    uint8_t frameCount;
    uint8_t paddingFrameCount;
    int size;
    double startTime;
    double endTime;

    AudioBuffer(){
        data = NULL;
        startPTS = -1;
        endPTS  = -1;
        frameCount = -1;
        paddingFrameCount = -1;
        size = -1;
        startTime=-1;
        endTime =-1;
    }

    ~AudioBuffer(){
        if(data)
            delete[] data;
    }
};

struct Audio{
    uint8_t* data;
    int size;
    int frameNumber;
    int stream;

    Audio(){
        data = NULL;
        size = -1;
        frameNumber = -1;
        stream = -1;
    }

    void clean(){
        if(data)
            delete[] data;
    }
};

constexpr QAudioFormat::SampleFormat mapSampleFormat(AVSampleFormat sampleFormat)
{
    switch (sampleFormat)
    {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_U8P:
        return QAudioFormat::UInt8;
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
        return QAudioFormat::Int16;
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
        return QAudioFormat::Int32;
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
    case AV_SAMPLE_FMT_DBL:
    case AV_SAMPLE_FMT_DBLP:
        return QAudioFormat::Float;
    case AV_SAMPLE_FMT_NONE:
    default:
        return QAudioFormat::Unknown;
    }
}

struct AudioStreamInfo{
    int sampleRate;
    int channelCount;
    int sampleCount;
    AVSampleFormat format;
    AVSampleFormat packedFormat;
    /* to do */
    /* add duration of audio steamm */
};

struct VideoStreamInfo{
    int frameRate;
    int width;
    int height;
    AVPixelFormat pixFmt;
    int frameCount;
};

struct VideoFileInfo{
    int numberStreams;
    int numberAudioStreams;
    int numberVideoStreams;
    VideoFileInfo() : numberStreams(0),numberAudioStreams(0),numberVideoStreams(0){}
};


enum ImageFromat {RGBA8,RGB8,RGB10,RGBA10};

class Video{
public:
    Video(const char* path);

    bool open();

    bool decodeVideo(int streamIndex,int64_t frameNumber, VideoFrame& videoFrame);
    bool getAudioStreamInfo(int streamIndex,AudioStreamInfo& info);
    bool getVideoStreamInfo(int streamIndex,VideoStreamInfo& info);
    bool getVideoFileInfo(VideoFileInfo& info);

    void getAudio(int streamIndex, int64_t frameNumber, double start, double end, Audio& audio);

    double getFrameRate(){
        return m_frameRate;
    }




private:
    const char* m_path;
    const char* m_fileName;

    AudioBuffer m_audioBuffer;
    int m_bufferTime = 10;
    int m_bufferPaddingTime = 5;

    //video
    int width;
    int height;
    double m_frameRate;
    int startTime;
    int frameCount;
    double duration;
    int timebase;
    //AVRational timeBase;
    //uint8_t* imageBuffer;
    int64_t pts;
    std::vector<int> videoStreamIndexes;
    int numVideoStreams = 0;
    std::vector<int> audioStreamIndexes;
    std::unordered_map<quint64,AVCodecContext*> codecContexts;
    std::unordered_map<quint64,SwsContext*> scalerContexts;
    std::unordered_map<quint64,SwrContext*> resampleContexts;
    std::unordered_map<int,quint64> streamIndexes;
    int numAudioStreams = 0;
    quint64 nextId = 0;


    //ffmpeg
    AVFormatContext* formatContext;
    AVFrame* frame;
    //AVFrame* Hwframe;
    AVPacket* packet;
    //AVHWDeviceType hw_device_type = AV_HWDEVICE_TYPE_DXVA2;
    //AVBufferRef* hw_device_ctx = nullptr;


    std::vector<AudioFrame> decodeAudio(int streamIndex, double startTime, std::vector<AudioFrame> &audioFrames);

    bool isAudioBuffered(int streamIndex, double time);

    void bufferAudio(int streamIndex,double start);

    static void moveToAudioBuffer(AudioBuffer &buffer, std::vector<AudioFrame> &frames, int padding);
    quint64 assignStreamId(int streamIndex,AVFormatContext* format);

};

#endif // VIDEO_H
