#include "viewerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <vulkan/vulkanwidget.h>

ViewerWidget::ViewerWidget(VulkanWindow* vulkanWindow,QWidget *parent)
    : QWidget{parent}
{
    m_vulkanWindow = vulkanWindow;

    vulkanWidget = new VulkanWidget(m_vulkanWindow);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_label = new QLabel();
    m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_label->setMinimumHeight(100);

    vulkanWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QImage img(parent->height(),parent->height(),QImage::Format_RGB888);
    img.fill(Qt::black);
    m_lastImage = img;
    QPixmap pixmap =  QPixmap::fromImage(img);
    m_label->setPixmap(pixmap);

    m_playButton = new QPushButton(this);
    m_playButton->setCheckable(true);
    m_playButton->setChecked(false);
    m_playButton->setText("Play/Pause");

    QHBoxLayout *viewerlayout = new QHBoxLayout();
    QSpacerItem* leftSpacer = new QSpacerItem(1,1,QSizePolicy::Expanding, QSizePolicy::Minimum);

    QSpacerItem* RightSpacer = new QSpacerItem(1,1,QSizePolicy::Expanding, QSizePolicy::Minimum);

    QSpacerItem* bottomSpacer = new  QSpacerItem(1, 5, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QSpacerItem* topSpacer = new  QSpacerItem(1, 5, QSizePolicy::Minimum, QSizePolicy::Expanding);

    layout->addItem(viewerlayout);
    viewerlayout->addSpacerItem(leftSpacer);
    viewerlayout->addWidget(vulkanWidget);
    viewerlayout->setAlignment(vulkanWidget,Qt::AlignCenter);
    viewerlayout->addSpacerItem(RightSpacer);
    m_playButton->setMaximumWidth(100);
    layout->addWidget(m_playButton);
    layout->setAlignment(m_playButton,Qt::AlignCenter);
    layout->setContentsMargins(0, 20, 0, 20);
    layout->setSpacing(20);

    viewerlayout->setContentsMargins(0, 20, 0, 0);
    viewerlayout->setSpacing(20);

    setLayout(layout);
    QObject::connect(m_playButton,&QPushButton::toggled,this,&ViewerWidget::playPause);

}

void ViewerWidget::setSrcRes(int width, int height)
{
    m_srcWidth = width;
    m_srcHeight = height;
}

void ViewerWidget::setImage(VideoFrame frame)
{
    lastFrame.clean();

    QPixmap surface = m_label->pixmap();
    QImage img;
    if(frame.height!=-1)
         img = QImage((uchar*)frame.frameData, frame.width, frame.height, QImage::Format_RGBA8888);
    else{
        img = QImage(m_label->pixmap().width(), m_label->pixmap().height(), QImage::Format_RGBA8888);
        img.fill(Qt::black);
    }
    m_lastImage = img;
    QImage scaled = img.scaled(surface.width(),surface.height(), Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    m_label->setPixmap(QPixmap::fromImage(scaled));
    lastFrame=frame;
}

void ViewerWidget::scalePixmap()
{
    int width = size().width();
    int height = size().height()-m_playButton->height()-100;

    double scaleWidth = (double)width/(double)m_srcWidth;
    double scaleHeight = (double)height/(double)m_srcHeight;
    double scale = width>height ? scaleHeight : scaleWidth;
    height = scale* m_srcHeight;
    width = scale * m_srcWidth;

    QImage scaled = m_lastImage.scaled(width,height, Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

    QPixmap pixmap = QPixmap::fromImage(scaled);
    m_label->setPixmap(pixmap);

    vulkanWidget->updateSizeHint(width,height);


}

void ViewerWidget::playPause(bool b)
{
    if(b)
        emit play();
    else
        emit pause();

}

void ViewerWidget::resizeEvent(QResizeEvent *event)
{

    QWidget::resizeEvent(event);
    scalePixmap();
}
