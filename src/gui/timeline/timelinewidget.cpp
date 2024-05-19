#include "timelinewidget.h"
#include <QHBoxLayout>

TimelineWidget::TimelineWidget(QWidget *parent)
    : QFrame(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QSplitter* splitter = new QSplitter(Qt::Horizontal,this);
    layout->addWidget(splitter);
    TimelineModel* model = new TimelineModel();
    model->createTrack(MediaType::VIDEO);
    model->createTrack(MediaType::AUDIO);

    // track pos in out
    model->addClip(0,0,0,30);
    model->addClip(1,0,0,30);

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


}

TimelineWidget::~TimelineWidget()
{

}

void TimelineWidget::getFrames(std::vector<std::pair<const ClipModel *, int> >)
{
 /* TO DO */
}

