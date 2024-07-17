#ifndef VULKANWIDGET_H
#define VULKANWIDGET_H

#include <QObject>
#include <QWidget>
#include "vulkanwindow.h"

class VulkanWidget : public QWidget
{
    Q_OBJECT
public:
    VulkanWidget(VulkanWindow* window, QWidget *parent = nullptr);

    int sizeX=500;
    int sizeY=500;
    int m_srcWidth = 1920;
    int m_srcHeight = 1080;
    VulkanWindow* m_vulkanWindow;

    void updateSizeHint(int width,int height);

signals:

    // QWidget interface
public:
    QSize sizeHint() const override;



    // QWidget interface
protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // VULKANWIDGET_H
