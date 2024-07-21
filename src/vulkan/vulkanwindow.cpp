#include "vulkanwindow.h"

#include <QResizeEvent>
#include "vk_initializers.h"
#include "vk_images.h"

VulkanWindow::VulkanWindow( vkb::Instance &vkbinstance) :  vkbinstance(vkbinstance)
{
    m_renderer = new VulkanRenderer(this);
    setMouseGrabEnabled(true);
    this->instance.setVkInstance(vkbinstance.instance);
    instance.create();
    setVulkanInstance(&instance);
    setSurfaceType(QSurface::VulkanSurface);
    setVulkanInstance(&this->instance);

    show();
    _surface = QVulkanInstance::surfaceForWindow(this);

    initVulkanWindow();
    initSwapChain();
    initCommands();
    initSyncStructures();

    m_renderer->init();

    ready = true;
    //draw();

}

void VulkanWindow::initVulkanWindow()
{
    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{};
    features.dynamicRendering = true;
    features.synchronization2 = true;

    //vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    //use vkbootstrap to select a gpu.
    //We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
    vkb::PhysicalDeviceSelector selector{vkbinstance};
    vkb::PhysicalDevice physicalDevice = selector
                                             .set_minimum_version(1, 3)
                                             .set_required_features_13(features)
                                             .set_required_features_12(features12)
                                             .set_surface(_surface)
                                             .select()
                                             .value();


    //create the final vulkan device
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };

    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a vulkan application
    device = vkbDevice.device;
    gpu = physicalDevice.physical_device;
    qDebug()<<physicalDevice.name;
    // use vkbootstrap to get a Graphics queue
    graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanWindow::initSwapChain()
{
    createSwapChain(width(),height());
}

void VulkanWindow::createSwapChain(int width, int height)
{
    vkb::SwapchainBuilder swapchainBuilder{ gpu, device,_surface };

    _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
                                      //.use_default_format_selection()
                                      .set_desired_format(VkSurfaceFormatKHR{ .format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
                                      //use vsync present mode
                                      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)//VK_PRESENT_MODE_IMMEDIATE_KHR//VK_PRESENT_MODE_FIFO_KHR
                                      .set_desired_extent(width, height)
                                      .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                      .build()
                                      .value();

    _swapchainExtent = vkbSwapchain.extent;
    //store swapchain and its related images
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanWindow::initCommands()
{
    //create a command pool for commands submitted to the graphics queue.
    //we also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
    }
}

void VulkanWindow::startFrame()
{
    if(m_framePending || !m_renderer)
        return;
    m_framePending = true;
    // wait until the gpu has finished rendering the last frame. Timeout of 1
    // second
    VK_CHECK(vkWaitForFences(device, 1, &get_current_frame()._renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(device, 1, &get_current_frame()._renderFence));

    //request image from the swapchain
    uint32_t swapchainImageIndex;

    VkResult e = vkAcquireNextImageKHR(device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex);
    if (e == VK_ERROR_OUT_OF_DATE_KHR) {
        //resize_requested = true;
        return ;
    }
    m_currentSwapchainImageIndex = swapchainImageIndex;

    //naming it cmd for shorter writing
    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

    // now that we are sure that the commands finished executing, we can safely
    // reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    //begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd,&cmdBeginInfo));
    start = std::chrono::high_resolution_clock::now();
    m_renderer->startNextFrame();
}

void VulkanWindow::endFrame()
{
    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;
    uint32_t swapchainImageIndex = m_currentSwapchainImageIndex;
    VkResult e;

    VK_CHECK(vkEndCommandBuffer(cmd));

    //prepare the submission to the queue.
    //we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
    //we will signal the _renderSemaphore, to signal that rendering has finished

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,get_current_frame()._swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo,&signalInfo,&waitInfo);

    //submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, get_current_frame()._renderFence));

    //prepare present
    // this will put the image we just rendered to into the visible window.
    // we want to wait on the _renderSemaphore for that,
    // as its necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || e ==  VK_SUBOPTIMAL_KHR) {
        //resize_requested = true;
    }

    //increase the number of frames drawn
    _frameNumber++;
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> duration = end - start;
    float frameTimeMs = duration.count();
    //qDebug() << "Frame time:" << frameTimeMs << "ms";
    requestUpdate();
}

void VulkanWindow::frameReady()
{
    m_framePending = false;
    endFrame();
}

VkQueue VulkanWindow::getGraphicsQueue() const
{
    return graphicsQueue;
}

VkDevice VulkanWindow::getDevice() const
{
    return device;
}

VkPhysicalDevice VulkanWindow::getPhysicalDevice() const
{
    return gpu;
}

VulkanRenderer *VulkanWindow::renderer() const
{
    return m_renderer;
}



int8_t VulkanWindow::getCurrentSwapchainImageIndex() const
{
    return m_currentSwapchainImageIndex;
}

uint32_t VulkanWindow::getGraphicsQueueFamilyIndex() const
{
    return graphicsQueueFamilyIndex;
}


void VulkanWindow::resizeSwapchain(){
    vkDeviceWaitIdle(device);

    destroySwapchain();


    createSwapChain(width(), height());
}

void VulkanWindow::destroySwapchain(){
    vkDestroySwapchainKHR(device, _swapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < _swapchainImageViews.size(); i++) {

        vkDestroyImageView(device, _swapchainImageViews[i], nullptr);
    }
}

void VulkanWindow::initSyncStructures()
{
    //create syncronization structures
    //one fence to control when the gpu has finished rendering the frame,
    //and 2 semaphores to syncronize rendering with swapchain
    //we want the fence to start signalled so we can wait on it on the first frame
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
    }

    //VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immFence));
}



void VulkanWindow::resizeEvent(QResizeEvent *e)
{

    resizeSwapchain();
    QWindow::resizeEvent(e);

}

void VulkanWindow::showEvent(QShowEvent *e)
{
    requestUpdate();
    QWindow::showEvent(e);
}

bool VulkanWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::UpdateRequest:
        //draw();
        startFrame();
        break;
    }
    return QWindow::event(event);
}





