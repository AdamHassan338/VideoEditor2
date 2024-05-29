#include "timelinewidget.h"
#include <QHBoxLayout>
#include "video.h"

TimelineWidget::TimelineWidget(QWidget *parent)
    : QFrame(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QSplitter* splitter = new QSplitter(Qt::Horizontal,this);
    layout->addWidget(splitter);
    TimelineModel* model = new TimelineModel();
    //model->createTrack(MediaType::VIDEO);
    //model->createTrack(MediaType::AUDIO);

    // track pos in out
    //model->addClip(0,0,0,30);
    //model->addClip(1,0,0,30);

    TimelineView* view = new TimelineView(this);
    TracklistView* tracklist = new TracklistView(this);
    view->setModel(model);
    tracklist->setModel(model);

    QToolBar* toolbar = new QToolBar("zoom slider",view);
    QSlider* slider = new QSlider(Qt::Horizontal);

    slider->setRange(2, 50);
    slider->setValue(5);
    toolbar->addWidget(slider);

    splitter->addWidget(tracklist);
    splitter->addWidget(view);
    splitter->setHandleWidth(0);
    QList<int> sizes({120,780});
    splitter->setMouseTracking(true);


    //splitter->resize(880,230);
    splitter->setSizes(sizes);
    setLayout(layout);
    QObject::connect(slider,&QSlider::valueChanged,view,&TimelineView::setScale);
    QObject::connect(view,&TimelineView::scrolled,tracklist,&TracklistView::scroll);
    QObject::connect(tracklist,&TracklistView::scrolled,view,&TimelineView::scroll);

    QObject::connect(model,&TimelineModel::newClip,view,&TimelineView::addClipToMap);
    QObject::connect(model,&TimelineModel::trackMoved,view,&TimelineView::TrackMoved);

    QObject::connect(model,&TimelineModel::playheadMoved,tracklist,&TracklistView::setTime);
    QObject::connect(model,&TimelineModel::tracksChanged,tracklist,&TracklistView::updateViewport);


    QObject::connect(model,&TimelineModel::underPlayhead,this,&TimelineWidget::getFrames);

}

TimelineWidget::~TimelineWidget()
{

}

void TimelineWidget::getFrames(std::vector<std::pair<const ClipModel *, int>> clipItems)
{
    VideoFrame videoFrame;
    std::vector<AudioFrame> audioFrames;
    for(std::pair<const ClipModel *, int> &item : clipItems){
        if(item.first->type()== MediaType::VIDEO)
        item.first->video()->decodeVideo(item.first->streamIndex(),item.second,videoFrame);
        if(item.first->type()== MediaType::AUDIO)
            item.first->video()->decodeAudio(item.first->streamIndex(),item.second,audioFrames);

        if(videoFrame.height!=-1 && audioFrames.size()>0)
            break;
    }

    //if(frame.height==-1)
        //return;
    emit newImage(videoFrame);
    emit newAudioFrame(audioFrames);
    return;

 /* TO DO */
}

