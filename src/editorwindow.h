#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include "timelinewidget.h"
#include "viewerwidget.h"
#include <QTimer>

class QAudioSink;
class QIODevice;

class EditorWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit EditorWindow(QWidget *parent = nullptr);

public slots:
    void writeToAudioSink(Audio audio);
signals:
private:
    TimelineWidget* m_timelineWidget;
    ViewerWidget* m_viewerWidegt;

    QAudioSink *m_audioSink;
    QIODevice *m_audioOutput;
    QTimer m_timer;

};

#endif // EDITORWINDOW_H
