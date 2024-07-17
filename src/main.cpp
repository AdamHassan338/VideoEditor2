#include "video.h"
#include "editorwindow.h"
#include <QtMultimedia/QAudioSink>
#include <QtMultimedia/QAudioOutput>
#include <QLabel>
#include <QThread>

#include "qtreeview.h"
#include "timelinemodel.h"
#include "timelinewidget.h"
#include "timelineview.h"
#include "tracklistview.h"
#include "types.h"

#include <QApplication>
#include <QVulkanInstance>
#include <QToolBar>
#include<QStyleFactory>
#include <QSplitter>
#include <QPalette>
#include <QStyle>

#include "vulkan/vulkanwindow.h"
#include <QtVersion>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>




int main(int argc, char *argv[]){
    QColor bgColour = QColor("#262626");
    QColor fillColour = QColor("#202020");
    QColor seperatorColour = QColor("#313131");
    QColor rulerColour = QColor("#4F4F4F");
    QApplication app(argc,argv);
    qApp->setStyle(QStyleFactory::create("fusion"));
    qRegisterMetaType<MediaType>("MediaType");


    QVulkanInstance inst;
    inst.setLayers({ "VK_LAYER_KHRONOS_validation",VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME });
    //inst.setLayers({VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME });

    inst.setApiVersion(QVersionNumber(1, 3));


    if (!inst.create())
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());


    //qDebug() << inst.apiVersion();
    //qDebug() << inst.extensions();
    VulkanWindow vw;
    //w.setFormat(format);
    //w.setEnabledFeaturesModifier(modifyDeviceFeatures);


    vw.setVulkanInstance(&inst);


    //w.resize(1024, 768);
    //w.show();

    EditorWindow w(&vw);
    QPalette palette = w.palette();
    palette.setColor(QPalette::Window,"#262626");
    palette.setColor(QPalette::Button,"#555555");
    palette.setColor(QPalette::ButtonText, Qt::white);


    w.setPalette(palette);
    w.resize(1280, 720);
    w.show();
    return app.exec();

    /*Video v = Video("/home/adam/Videos/test.mp4");
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

    QLabel* label = new QLabel();
    label->setPixmap(QPixmap(900,900));
    label->show();
    list.resize(5000);

    for(int i = 0; i< 100; ++i){
        AudioFrame audioFrame;

        std::vector<AudioFrame> list = v.decodeAudio(0,i,audioFrame);
        //list.emplace_back(audioFrame);
        for(int j = 0; j<list.size();j++){
        if(list[j].frameSize<0)
        continue;
        //audioOutput->write(reinterpret_cast<char*>(list[j].frameData), list[j].frameSize);
        qDebug()<< "FREE : " <<audioSink->bytesFree();
        }


    }
    while(audioSink->state() != QAudio::IdleState){
        int a = 5;
    }
    qDebug() << "stopped";
    for(int i = 399; i< 600; ++i){
        AudioFrame audioFrame;

        std::vector<AudioFrame> list = v.decodeAudio(0,i,audioFrame);
        //list.emplace_back(audioFrame);
        for(int j = 0; j<list.size();j++){
            if(list[j].frameSize<0)
                continue;
            //audioOutput->write(reinterpret_cast<char*>(list[j].frameData), list[j].frameSize);
            qDebug()<< "FREE : " <<audioSink->bytesFree();
        }


    }
//    QThread::sleep(500000);


   for(int i = 0; i< 240; i++){

        VideoFrame frame;
        v.decodeVideo(0,4,frame);
        QImage img((uchar*)frame.frameData, frame.width, frame.height, QImage::Format_RGBA8888);
        QPixmap pixmap = QPixmap::fromImage(img);
        label->setPixmap(pixmap);
        QApplication::processEvents();


    }









    return app.exec();*/
}
