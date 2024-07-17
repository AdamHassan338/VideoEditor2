#include "editorwindow.h"
#include <QVBoxLayout>
#include <QSplitter>
#include <QIODevice>
#include <QAudioSink>
#include <QAudioSource>

EditorWindow::EditorWindow(VulkanWindow* vulkanWindow,QWidget *parent)
    : QMainWindow{parent}
{

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    QWidget* centralWidget = new QWidget(this);
    QSplitter* splitter = new QSplitter(Qt::Vertical,this);


    m_timelineWidget = new TimelineWidget(centralWidget);
    m_viewerWidegt = new ViewerWidget(vulkanWindow,this);
    setCentralWidget(centralWidget);
    splitter->addWidget(m_viewerWidegt);
    splitter->addWidget(m_timelineWidget);
    centralWidget->setLayout(layout);
    layout->addWidget(splitter);



    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Float);


    m_audioSink = new QAudioSink(format,this);
    //m_audioOutput = m_audioSink->start();

    QObject::connect(m_timelineWidget,&TimelineWidget::newImage,m_viewerWidegt,&ViewerWidget::setImage);
    QObject::connect(m_timelineWidget,&TimelineWidget::newAudioFrame,this,&EditorWindow::writeToAudioSink);
    QObject::connect(&m_timer,&QTimer::timeout,this,[&]() {
        if(m_audioSink->state()==QAudio::IdleState)
            m_audioSink->stop();
        });

    QObject::connect(m_viewerWidegt,&ViewerWidget::play,m_timelineWidget,&TimelineWidget::play);

    QObject::connect(m_viewerWidegt,&ViewerWidget::pause,m_timelineWidget,&TimelineWidget::pause);

}

void EditorWindow::writeToAudioSink(Audio audio)
{   if(m_audioSink->state()==QAudio::StoppedState  || m_audioSink->error() !=QAudio::NoError )
        m_audioOutput = m_audioSink->start();
    m_audioOutput->write(reinterpret_cast<char*>(audio.data), audio.size);
    qDebug() << audio.size;
    audio.clean();
    m_timer.start(200);
}
