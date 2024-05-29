#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QFrame>
#include <QSplitter>
#include <QToolBar>

#include "timelinemodel.h"
#include "timelinewidget.h"
#include "timelineview.h"
#include "tracklistview.h"
#include "types.h"
#include "video.h"


class TimelineWidget : public QFrame
{
    Q_OBJECT

public:
    TimelineWidget(QWidget *parent = nullptr);
    ~TimelineWidget();

signals:
    void newImage(VideoFrame frame);
    void newAudioFrame(std::vector<AudioFrame> audioFrames);

public slots:
    void getFrames(std::vector<std::pair<const ClipModel*, int>> clipItems);

private:
    TimelineView* m_timelineView;
    TracklistView* m_tracklistView;
    TimelineModel* m_model;
    QSplitter* m_splitter;
};
#endif // TIMELINEWIDGET_H
