#include "video.h"
#include <QtMultimedia/QAudioSink>
#include <QtMultimedia/QAudioOutput>

int main(int argc, char *argv[]){

    QApplication app(argc,argv);

    Video v = Video("/home/adam/Downloads/Dad_who_was_romen_REAL_2.mp4");
    if(!v.open())
        return -1;

    AudioStreamInfo info;

    if(!v.getAudioStreamInfo(0,info))
        return -1;

    QAudioFormat format;
    format.setSampleRate(info.sampleRate);
    format.setChannelCount(info.channelCount);
    format.setSampleFormat(mapSampleFormat(info.packedFormat));


    QAudioSink *audioSink = new QAudioSink(format,nullptr);
    QIODevice *audioOutput = audioSink->start();

    std::vector<AudioFrame> list;

    for(int i = 0; i< 600; ++i){
        AudioFrame audioFrame;
        v.decodeAudio(0,i,audioFrame);
        list.push_back(audioFrame);
        if(list[i].frameSize<0)
            continue;
        audioOutput->write(reinterpret_cast<char*>(list[i].frameData), list[i].frameSize);

    }

    return app.exec();
}
