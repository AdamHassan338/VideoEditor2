#include "editorwindow.h"
#include <QVBoxLayout>
#include <QSplitter>

EditorWindow::EditorWindow(QWidget *parent)
    : QMainWindow{parent}
{

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    QWidget* centralWidget = new QWidget(this);
    QSplitter* splitter = new QSplitter(Qt::Vertical,this);


    m_timelineWidget = new TimelineWidget(centralWidget);
    m_viewerWidegt = new ViewerWidget(this);
    setCentralWidget(centralWidget);
    splitter->addWidget(m_viewerWidegt);
    splitter->addWidget(m_timelineWidget);
    centralWidget->setLayout(layout);
    layout->addWidget(splitter);

    QObject::connect(m_timelineWidget,&TimelineWidget::newImage,m_viewerWidegt,&ViewerWidget::setImage);

}
