#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QFrame>
#include <QSplitter>
#include <QToolBar>

#include "timelinemodel.h"
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
    void newAudioFrame(Audio audio);

public slots:
    void getFrames(std::vector<std::pair<const ClipModel*, int>> clipItems);
    void play();
    void pause();
    void movePlayhead();

private:
    TimelineView* m_timelineView;
    TracklistView* m_tracklistView;
    TimelineModel* m_model;
    QSplitter* m_splitter;
    QTimer* m_playTimer;
};
#endif // TIMELINEWIDGET_H
