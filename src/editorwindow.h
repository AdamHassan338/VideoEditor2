#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include "timelinewidget.h"
#include "viewerwidget.h"

class EditorWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit EditorWindow(QWidget *parent = nullptr);

signals:
private:
    TimelineWidget* m_timelineWidget;
    ViewerWidget* m_viewerWidegt;
};

#endif // EDITORWINDOW_H