#include "viewerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
ViewerWidget::ViewerWidget(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_label = new QLabel(this);
   // m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_label->setMinimumSize(100,100);

    QImage img(parent->height(),parent->height(),QImage::Format_RGB888);
    img.fill(Qt::black);
    QPixmap pixmap =  QPixmap::fromImage(img);
    m_label->setPixmap(pixmap);

    m_playButton = new QPushButton(this);
    m_playButton->setText("Play/Pause");

    QHBoxLayout *viewerlayout = new QHBoxLayout();
    QSpacerItem* leftSpacer = new QSpacerItem(1,1,QSizePolicy::Expanding, QSizePolicy::Minimum);

    QSpacerItem* RightSpacer = new QSpacerItem(1,1,QSizePolicy::Expanding, QSizePolicy::Minimum);

    QSpacerItem* bottomSpacer = new  QSpacerItem(1, 5, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QSpacerItem* topSpacer = new  QSpacerItem(1, 5, QSizePolicy::Minimum, QSizePolicy::Expanding);

    layout->addItem(viewerlayout);
    viewerlayout->addSpacerItem(leftSpacer);
    viewerlayout->addWidget(m_label);
    viewerlayout->setAlignment(m_label,Qt::AlignCenter);
    viewerlayout->addSpacerItem(RightSpacer);
    m_playButton->setMaximumWidth(100);
    layout->addWidget(m_playButton);
    layout->setAlignment(m_playButton,Qt::AlignCenter);
    layout->setContentsMargins(0, 20, 0, 20);
    layout->setSpacing(20);

    viewerlayout->setContentsMargins(0, 20, 0, 0);
    viewerlayout->setSpacing(20);

    setLayout(layout);

}

void ViewerWidget::setSrcRes(int width, int height)
{
    m_srcWidth = width;
    m_srcHeight = height;
}

void ViewerWidget::scalePixmap()
{
    int width = size().width();
    int height = size().height()-m_playButton->height();
    double scaleWidth = (double)width/(double)m_srcWidth;
    double scaleHeight = (double)height/(double)m_srcHeight;
    double scale = std::min(scaleWidth,scaleHeight);
    height = scale* m_srcHeight;
    width = scale * m_srcWidth;
    QImage img(width, height, QImage::Format_RGB888);
    img.fill(Qt::black);
    QPixmap pixmap = QPixmap::fromImage(img);
    m_label->setPixmap(pixmap);
}

void ViewerWidget::resizeEvent(QResizeEvent *event)
{

    QWidget::resizeEvent(event);
    scalePixmap();
}
