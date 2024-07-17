#include "vulkanwidget.h"
#include <QVBoxLayout>
#include <QResizeEvent>

VulkanWidget::VulkanWidget(VulkanWindow *window, QWidget *parent)
    : QWidget{parent}
{
    m_vulkanWindow = window;
    QWidget* wrapper =  QWidget::createWindowContainer(window,this);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(wrapper);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    setLayout(layout);
    setMinimumSize(100,100/(16.0/9.0));
}

void VulkanWidget::updateSizeHint(int width, int height){
    sizeX = width;
    sizeY=height;
    updateGeometry();
}

QSize VulkanWidget::sizeHint() const
{
    // called by layout when reducing size
    return QSize(sizeX,sizeY);
}

void VulkanWidget::resizeEvent(QResizeEvent *event)
{
    // called by layout when increasing size

    int width = event->size().width();
    int height = event->size().height();

    double scaleWidth = (double)width/(double)m_srcWidth;
    double scaleHeight = (double)height/(double)m_srcHeight;
    double scale = width>height ? scaleHeight : scaleWidth;
    height = scale* m_srcHeight;
    width = scale * m_srcWidth;

    sizeX = width;
    sizeY = height;
    updateGeometry();

    QWidget::resizeEvent(event);

}
