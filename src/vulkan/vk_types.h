#ifndef VK_TYPES_H
#define VK_TYPES_H

#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>



#define VK_CHECK(x)                                                     \
do {                                                                \
        VkResult err = x;                                               \
        if (err != VK_SUCCESS) {                                        \
            qCritical() << "Detected Vulkan error:" << string_VkResult(err); \
            abort();                                                    \
    }                                                               \
} while (0)


struct MaterialPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

enum class MaterialPass :uint8_t {
    MainColor,
    Transparent,
    Other
};

struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet materialSet;
    MaterialPass passType;
};

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct Vertex {

    QVector3D position;
    float uv_x;
    QVector3D normal;
    float uv_y;
    QVector4D color;
};

// holds the resources needed for a mesh
struct GPUMeshBuffers {

    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
};

// push constants for our mesh object draws
struct GPUDrawPushConstants {
    //QMatrix4x4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};

#endif // VK_TYPES_H
