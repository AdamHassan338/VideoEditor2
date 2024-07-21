#ifndef VULKANWINDOW_H
#define VULKANWINDOW_H
#pragma once
#include <QWindow>
#include <vulkan/vulkan.h>
#include <QVulkanInstance>
#include "VkBootstrap.h"
#include"vulkanrenderer.h"
#include <chrono>

constexpr unsigned int FRAME_OVERLAP = 2;

struct FrameData {
    VkSemaphore _swapchainSemaphore, _renderSemaphore;
    VkFence _renderFence;

    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;

    //DeletionQueue _deletionQueue;
    //DescriptorAllocatorGrowable _frameDescriptors;
};

class VulkanWindow : public QWindow
{
    Q_OBJECT


public:
    VulkanWindow(vkb::Instance &vkbinstance);
    void initVulkanWindow();
    void initSwapChain();
    void createSwapChain(int width, int height);
    void initCommands();
    void initSyncStructures();


    void startFrame();
    // make private
    void endFrame();
    void frameReady();

    bool m_framePending = false;

    int _frameNumber {0};

    FrameData _frames[FRAME_OVERLAP];

    FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; }


    VkQueue getGraphicsQueue() const;

    VkDevice getDevice() const;

    VkImage getCurrentSwapchainImage(){
        return _swapchainImages[m_currentSwapchainImageIndex];
    };
    VkImageView getCurrentSwapchainImageView(){
        return _swapchainImageViews[m_currentSwapchainImageIndex];
    };

    VkCommandBuffer getCurrentCommandBuffer(){
        return get_current_frame()._mainCommandBuffer;
    }
    VkExtent2D swapChainImageSize(){
        return _swapchainExtent;
    }
    VkPhysicalDevice getPhysicalDevice() const;

    uint32_t getGraphicsQueueFamilyIndex() const;

    int8_t getCurrentSwapchainImageIndex() const;
    VulkanRenderer *renderer() const;

private:
    std::chrono::high_resolution_clock::time_point start;
    bool ready = false;
    VulkanRenderer* m_renderer = nullptr;
    QVulkanInstance instance;
    vkb::Instance vkbinstance;
    VkSurfaceKHR _surface;
    VkDevice device;

    int8_t m_currentSwapchainImageIndex =-1;

    VkPhysicalDevice gpu;

    VkQueue graphicsQueue;

    uint32_t graphicsQueueFamilyIndex;


    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImageFormat;

    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    VkExtent2D _swapchainExtent;

    void resizeSwapchain();
    void destroySwapchain();




    // QWindow interface
protected:
    void resizeEvent(QResizeEvent *) override;
    void showEvent(QShowEvent *) override;


    // QObject interface
public:
    bool event(QEvent *event) override;



};

#endif // VULKANWINDOW_H
