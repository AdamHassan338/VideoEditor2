#include "timelinewidget.h"
#include <QHBoxLayout>
#include "video.h"
#include <QTimer>

TimelineWidget::TimelineWidget(QWidget *parent)
    : QFrame(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QSplitter* splitter = new QSplitter(Qt::Horizontal,this);
    layout->addWidget(splitter);
    m_model = new TimelineModel();

    m_timelineView = new TimelineView(this);
    m_tracklistView = new TracklistView(this);
    m_timelineView->setModel(m_model);
    m_tracklistView->setModel(m_model);

     m_playTimer = new QTimer();

    QToolBar* toolbar = new QToolBar("zoom slider",m_timelineView);
    QSlider* slider = new QSlider(Qt::Horizontal);

    slider->setRange(2, 50);
    slider->setValue(5);
    toolbar->addWidget(slider);

    splitter->addWidget(m_tracklistView);
    splitter->addWidget(m_timelineView);
    splitter->setHandleWidth(0);
    QList<int> sizes({120,780});
    splitter->setMouseTracking(true);


    //splitter->resize(880,230);
    splitter->setSizes(sizes);
    setLayout(layout);
    QObject::connect(slider,&QSlider::valueChanged,m_timelineView,&TimelineView::setScale);
    QObject::connect(m_timelineView,&TimelineView::scrolled,m_tracklistView,&TracklistView::scroll);
    QObject::connect(m_tracklistView,&TracklistView::scrolled,m_timelineView,&TimelineView::scroll);

    QObject::connect(m_model,&TimelineModel::newClip,m_timelineView,&TimelineView::addClipToMap);
    QObject::connect(m_model,&TimelineModel::trackMoved,m_timelineView,&TimelineView::TrackMoved);

    QObject::connect(m_model,&TimelineModel::playheadMoved,m_tracklistView,&TracklistView::setTime);
    QObject::connect(m_model,&TimelineModel::tracksChanged,m_tracklistView,&TracklistView::updateViewport);


    QObject::connect(m_model,&TimelineModel::underPlayhead,this,&TimelineWidget::getFrames);
    QObject::connect(m_playTimer,&QTimer::timeout,this,&TimelineWidget::movePlayhead);

}

TimelineWidget::~TimelineWidget()
{

}

void TimelineWidget::getFrames(std::vector<std::pair<const ClipModel *, int>> clipItems)
{
    VideoFrame videoFrame;
    Audio audio;
    for(std::pair<const ClipModel *, int> &item : clipItems){
        if(item.first->type()== MediaType::VIDEO)
        item.first->video()->decodeVideo(item.first->streamIndex(),item.second,videoFrame);
        if(item.first->type()== MediaType::AUDIO){
            //frame comes in with timeline number
            int frame = item.second;
            double timelineFrameRate = m_model->data(QModelIndex(),TimelineModel::TimelineFrameRateRole).toDouble();
            double time = frame/timelineFrameRate;
            double endtime = (frame+1)/timelineFrameRate;
            //real frame based on video stream
            frame = std::floor(frame/timelineFrameRate *item.first->video()->getFrameRate());
            item.first->video()->getAudio(item.first->streamIndex(),frame,time,endtime,audio);
        }
    }

    //if(frame.height==-1)
        //return;
    if(audio.size>0)
        emit newAudioFrame(audio);
    emit newImage(videoFrame);

    return;

 /* TO DO */
}

void TimelineWidget::play()
{
    if(m_playTimer == nullptr)
        m_playTimer = new QTimer();
    int frameTime = (1/m_model->data(QModelIndex(),TimelineModel::TimelineFrameRateRole).toDouble()) * 1000;
    m_playTimer->setInterval(frameTime);
    m_playTimer->start();
}

void TimelineWidget::pause()
{
    if(!m_playTimer)
        return;
    m_playTimer->stop();
}

void TimelineWidget::movePlayhead()
{
    m_timelineView->moveForward();

}

