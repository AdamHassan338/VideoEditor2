#include "vulkanwindow.h"
#include "vulkanrenderer.h"

void modifyDeviceFeatures(VkPhysicalDeviceFeatures2 &features) {

    // Enable dynamic rendering feature
    static VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures = {};
    dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

    // Enable synchronization2 feature
    static VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2Features = {};
    synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    synchronization2Features.synchronization2 = VK_TRUE;

    // Enable buffer device address feature
    static VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {};
    bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

    // Enable graphics pipeline library feature
    static VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT graphicsPipelineLibraryFeaturesEXT = {};
    graphicsPipelineLibraryFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT;
    graphicsPipelineLibraryFeaturesEXT.graphicsPipelineLibrary = VK_TRUE;

    // Chain the features
    if (features.pNext) {
        void* pNext = features.pNext;
        dynamicRenderingFeatures.pNext = pNext;
        synchronization2Features.pNext = &dynamicRenderingFeatures;
        bufferDeviceAddressFeatures.pNext = &synchronization2Features;
        features.pNext = &bufferDeviceAddressFeatures;
    } else {
        synchronization2Features.pNext = &dynamicRenderingFeatures;
        bufferDeviceAddressFeatures.pNext = &synchronization2Features;
        features.pNext = &bufferDeviceAddressFeatures;
    }

}


QVulkanWindowRenderer *VulkanWindow::createRenderer()
{

    setEnabledFeaturesModifier(modifyDeviceFeatures);
    setDeviceExtensions({VK_KHR_SWAPCHAIN_EXTENSION_NAME,VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME});

    QByteArrayList extensions;
    extensions << VK_KHR_SWAPCHAIN_EXTENSION_NAME
               << VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME;
    return new VulkanRenderer(this);
}

