#ifndef VULKANWINDOW_H
#define VULKANWINDOW_H

#include <QVulkanWindow>
#include <QVulkanWindowRenderer>
class VulkanRenderer;
class VulkanWindow : public QVulkanWindow
{
    Q_OBJECT

public:
    QVulkanWindowRenderer *createRenderer() override;
    VulkanRenderer* renderer;

private:

};

#endif // VULKANWINDOW_H
