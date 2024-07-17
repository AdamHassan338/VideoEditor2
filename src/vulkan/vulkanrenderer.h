#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QVulkanWindowRenderer>
#include "vulkanwindow.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include "vk_types.h"
#include "vk_descriptors.h"
#include <span>

struct GPUSceneData {
    float scale;
    float rotation;
};
struct TransformData {
    float scale = 1.f;
    float rotation = 0.f;
};
void transitionImage(VkCommandBuffer cmd, QVulkanDeviceFunctions* dev ,VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

void copy_image_to_image(VkCommandBuffer cmd, QVulkanDeviceFunctions *dev, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
class VulkanRenderer : public QVulkanWindowRenderer
{
public:
    VulkanRenderer(VulkanWindow* window);

    // QVulkanWindowRenderer interface
public:
    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void startNextFrame() override;

    VmaAllocator m_allocator;



    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
    {
        // allocate buffer
        VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.pNext = nullptr;
        bufferInfo.size = allocSize;

        bufferInfo.usage = usage;

        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = memoryUsage;
        vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        AllocatedBuffer newBuffer;

        // allocate the buffer
        VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation,
                                 &newBuffer.info));

        return newBuffer;
    }

    void destroy_buffer(const AllocatedBuffer& buffer)
    {
        vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
    }
    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);


private:
    QVulkanWindow* m_window;
    QVulkanDeviceFunctions *m_devFuncs;

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    QMatrix4x4 m_proj;
    float m_rotation = 0.0f;

    VkSampler m_linearSampler;
    VkSampler m_nearestSampler;

    VkShaderModule m_vertexShader;
    VkShaderModule m_fragmentShader;

    VkPipeline m_meshPipeline;
    VkPipelineLayout m_meshPipelineLayout;

    DescriptorAllocatorGrowable globalDescriptorAllocator;

    VkDescriptorSetLayout m_TransformDataDescriptorLayout;
    VkDescriptorSetLayout m_singleImageDescriptorLayout;
    VkDescriptorSetLayout m_materialLayout;

    std::vector<VkDescriptorSet> m_frameDescriptor;

    GPUMeshBuffers rectangle;
    GPUSceneData scenedata;
    void initpipeline();
    void initdiscriptors();
    // immediate submit structures
    VkFence m_imFence;
    VkCommandBuffer m_imCommandBuffer;
    VkCommandPool m_imCommandPool;
    AllocatedImage loadImage(std::string path );
    AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
    GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

    AllocatedImage m_image;
    AllocatedImage m_image2;
    int m_nextImage = 0;

    //frame stuff
    AllocatedBuffer gpuSceneDataBuffer;
    GPUSceneData* sceneUniformData;

    AllocatedBuffer m_transformDataBuffer;
    TransformData m_transformData;
    TransformData* m_transformDataUniform;




    VkDescriptorSet m_transformDescriptor;
    VkDescriptorSet m_singleImageDescriptor;

    void update();
private slots:
   // swapImage()

    // QVulkanWindowRenderer interface
public:
    void preInitResources() override;
};

#endif // VULKANRENDERER_H
