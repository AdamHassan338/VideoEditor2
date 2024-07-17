#ifndef VULKANWINDOW_H
#define VULKANWINDOW_H

#include <QVulkanWindow>
#include <QVulkanWindowRenderer>

class VulkanWindow : public QVulkanWindow
{
    Q_OBJECT

public:
    QVulkanWindowRenderer *createRenderer() override;

private:

};

#endif // VULKANWINDOW_H
