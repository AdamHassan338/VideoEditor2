#include "vulkanrenderer.h"

#include <QVulkanInstance>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <QRandomGenerator>
#include "stb_image.h"
#include "vk_pipelines.h"
#include "vk_images.h"
#include "vk_initializers.h"
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "vulkanwindow.h"


VulkanRenderer::VulkanRenderer(VulkanWindow *window) : m_window(window)
{

    //qDebug()<< m_window->supportedDeviceExtensions();
}

void VulkanRenderer::initdiscriptors(){
    //create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
        {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
        };

    globalDescriptorAllocator.init(m_window->getDevice(), 10, sizes);

    //make the descriptor set layout for scene data
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        m_TransformDataDescriptorLayout = builder.build(m_window->getDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        m_singleImageDescriptorLayout = builder.build(m_window->getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT);
    }



    //allocate a new uniform buffer for the scene data
    //AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    //add it to the deletion queue of this frame so it gets deleted once its been used


    //write the buffer
    //GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
    //*sceneUniformData = scenedata;
    m_transformDataBuffer = create_buffer(sizeof(TransformData),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,VMA_MEMORY_USAGE_CPU_TO_GPU);

    m_transformData.scale =1;
    m_transformDataUniform =(TransformData*)m_transformDataBuffer.allocation->GetMappedData();
    *m_transformDataUniform  = m_transformData;


    //create a descriptor set that binds that buffer and update it



    std::vector<VkDescriptorSetLayout> layouts;
    layouts.push_back(m_singleImageDescriptorLayout);
    layouts.push_back(m_TransformDataDescriptorLayout);

    m_frameDescriptor = globalDescriptorAllocator.allocate(m_window->getDevice(),layouts);
    {
        DescriptorWriter writer;
        //m_singleImageDescriptor = globalDescriptorAllocator.allocate(m_window->getDevice(),m_singleImageDescriptorLayout);
        writer.write_image(0,m_image.imageView,m_linearSampler,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.update_set(m_window->getDevice(),m_frameDescriptor[0]);
    }

    {
        DescriptorWriter writer;
        writer.write_buffer(0, m_transformDataBuffer.buffer, sizeof(TransformData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.update_set(m_window->getDevice(), m_frameDescriptor[1]);
    }


}

void VulkanRenderer::initpipeline(){

    //load shaders
    if(!vkutil::load_shader_module("./vulkan/shaders/tex_image.vert.spv",m_window->getDevice(),&m_vertexShader)){
        qDebug()<< "COULD NOT CREATE VERTEX SHADER";
    }
    if(!vkutil::load_shader_module("./vulkan/shaders/tex_image.frag.spv",m_window->getDevice(),&m_fragmentShader)){
        qDebug()<< "COULD NOT CREATE fragment SHADER";

    }

    // CREATE PIPELINE LAYOUT
    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(GPUDrawPushConstants);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.add_binding(0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    //layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    m_materialLayout = layoutBuilder.build(m_window->getDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayout layouts[] = { m_singleImageDescriptorLayout,
                                       m_TransformDataDescriptorLayout
                                        };

    VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
    mesh_layout_info.setLayoutCount = 2;
    mesh_layout_info.pSetLayouts = layouts;
    mesh_layout_info.pPushConstantRanges = &matrixRange;
    mesh_layout_info.pushConstantRangeCount = 1;

    VK_CHECK(vkCreatePipelineLayout(m_window->getDevice(), &mesh_layout_info, nullptr, &m_meshPipelineLayout));

    PipelineBuilder builder;
        builder._pipelineLayout = m_meshPipelineLayout;

    builder.set_shaders(m_vertexShader,m_fragmentShader);
    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    builder.set_cull_mode(VK_CULL_MODE_NONE,VK_FRONT_FACE_CLOCKWISE);
    builder.set_multisampling_none();
    builder.disable_blending();
    //builder.enable_depthtest(true,VK_COMPARE_OP_GREATER_OR_EQUAL);
    builder.disable_depthtest();
    //builder.set_color_attachment_format(VK_FORMAT_R8G8B8A8_SRGB);
    builder.set_color_attachment_format(VK_FORMAT_B8G8R8A8_SRGB);

    //pipelineBuilder.set_depth_format(_depthImage.imageFormat);
    m_meshPipeline = builder.build_pipeline(m_window->getDevice());
}

void VulkanRenderer::initResources()
{
    qDebug("initResources");

    VkDevice dev = m_window->getDevice();
    QVulkanInstance *inst = static_cast<QVulkanInstance*>(m_window->vulkanInstance());
    //m_devFuncs = inst->deviceFunctions(m_window->getDevice());


    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(inst->getInstanceProcAddr("vkGetInstanceProcAddr"));
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(inst->getInstanceProcAddr("vkGetDeviceProcAddr"));

    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;


    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_window->getPhysicalDevice();
    allocatorInfo.device = dev;
    allocatorInfo.instance = inst->vkInstance();
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;
    vmaCreateAllocator(&allocatorInfo, &m_allocator);

    VkDevice device = m_window->getDevice();
    VkQueue m_graphicsQueue = m_window->getGraphicsQueue();


    // Create imm fence
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;

    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(m_window->getDevice(), &fenceCreateInfo, nullptr, &m_imFence));

    //create imediate cmd pool
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(m_window->getGraphicsQueueFamilyIndex(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(m_window->getDevice(), &commandPoolInfo, nullptr, &m_imCommandPool));

    // allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_imCommandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(m_window->getDevice(), &cmdAllocInfo, &m_imCommandBuffer));


    //create samplers

    VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    sampl.magFilter = VK_FILTER_NEAREST;
    sampl.minFilter = VK_FILTER_NEAREST;

    vkCreateSampler(m_window->getDevice(), &sampl, nullptr, &m_nearestSampler);

    sampl.magFilter = VK_FILTER_LINEAR;
    sampl.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(m_window->getDevice(), &sampl, nullptr, &m_linearSampler);


    // Load image
    m_image = loadImage("/home/adam/Downloads/IMG_4918.jpg");
    // Load image
    m_image2 = loadImage("/home/adam/Downloads/APkrFKbupUzidYRMSttoZZW2GkJzvqxyxV7E5R-tmaorHAs176-c-k-c0x00ffffff-no-rj.png");

    VkExtent3D imageSize;
    imageSize.width =  1920;
    imageSize.height = 1080;
    imageSize.depth = 1;

    m_drawImage = create_image(imageSize,VK_FORMAT_B8G8R8A8_SRGB,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,false);

    initdiscriptors();
    initpipeline();

    //init mesh
    std::array<Vertex,4> rect_vertices;

    rect_vertices[0].position = {1,-1, 0};
    rect_vertices[1].position = {1,1, 0};
    rect_vertices[2].position = {-1,-1, 0};
    rect_vertices[3].position = {-1,1, 0};

    rect_vertices[0].uv_x = 1.0f;
    rect_vertices[0].uv_y = 0.0f;

    rect_vertices[1].uv_x = 1.0f;
    rect_vertices[1].uv_y = 1.0f;

    rect_vertices[2].uv_x = 0.0f;
    rect_vertices[2].uv_y = 0.0f;

    rect_vertices[3].uv_x = 0.0f;
    rect_vertices[3].uv_y = 1.0f;


    rect_vertices[0].color = {0,0, 0,1};
    rect_vertices[1].color = { 0.5,0.5,0.5 ,1};
    rect_vertices[2].color = { 1,0, 0,1 };
    rect_vertices[3].color = { 0,1, 0,1 };

    std::array<uint32_t,6> rect_indices;

    rect_indices[0] = 0;
    rect_indices[1] = 1;
    rect_indices[2] = 2;

    rect_indices[3] = 2;
    rect_indices[4] = 1;
    rect_indices[5] = 3;

    rectangle = uploadMesh(rect_indices,rect_vertices);



}

void VulkanRenderer::initSwapChainResources()
{

}

void VulkanRenderer::releaseSwapChainResources()
{

}

void VulkanRenderer::releaseResources()
{

}

void VulkanRenderer::startNextFrame()
{
    update();
    VkImage swapchainImage = m_window->getCurrentSwapchainImage();
    VkCommandBuffer cmd = m_window->getCurrentCommandBuffer();

    VkImageView view=  m_window->getCurrentSwapchainImageView();




    //VK_CHECK(vkResetCommandBuffer(cmd, 0));

   /* VkCommandBufferBeginInfo cmdBufBeginInfo = {};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;



    VkRenderingAttachmentInfo colorAttachment = {};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = m_window->swapChainImageView(m_window->currentSwapChainImageIndex());
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // Generate random color
    float r = QRandomGenerator::global()->bounded(1.0f);
    float g = QRandomGenerator::global()->bounded(1.0f);
    float b = QRandomGenerator::global()->bounded(1.0f);
    VkClearValue clearColor = {0, 0, 0, 1.0f};
    colorAttachment.clearValue = clearColor;

    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent.width = m_window->swapChainImageSize().width();
    renderingInfo.renderArea.extent.height = m_window->swapChainImageSize().height();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkutil::transition_image(cmd,swapchainImage,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL);

    // Begin command buffer DONE BY QT
    //vkBeginCommandBuffer(cmd, &cmdBufBeginInfo);

    // Begin dynamic rendering
    vkCmdBeginRendering(cmd, &renderingInfo);

    // End dynamic rendering
    vkCmdEndRendering(cmd);

    // End command buffer DONE BY QT
    //vkEndCommandBuffer(cmd);
    VkExtent2D swapSize;
    swapSize.height = m_window->swapChainImageSize().height();
    swapSize.width = m_window->swapChainImageSize().width();

    VkExtent2D imageSize;
    imageSize.height = m_image.imageExtent.height;
    imageSize.width = m_image.imageExtent.width;

    //vkutil::transition_image(cmd,m_image.image,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);


    //vkutil::transition_image(cmd,swapchainImage,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copy_image_to_image(cmd,m_image.image,swapchainImage,imageSize,swapSize );

    vkutil::transition_image(cmd,swapchainImage,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    //vkutil::transition_image(cmd,m_image.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    */
    VkClearValue clearColor = {0, 0, 0, 1.0f};

    VkExtent2D swapSize;
    VkExtent2D drawSize;
    swapSize.height = m_window->swapChainImageSize().height;
    swapSize.width = m_window->swapChainImageSize().width;
    drawSize.height = 1080;
    drawSize.width = 1920;

    VkRenderingAttachmentInfo colourAttachment = vkinit::attachment_info(m_drawImage.imageView,&clearColor,VK_IMAGE_LAYOUT_GENERAL);

    //VkRenderingAttachmentInfo colourAttachment = vkinit::attachment_info(view,&clearColor,VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingInfo renderInfo = vkinit::rendering_info(drawSize, &colourAttachment, nullptr);

    vkutil::transition_image(cmd,swapchainImage,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL);
    vkutil::transition_image(cmd,m_drawImage.image,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL);

    vkCmdBeginRendering(cmd,&renderInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,m_meshPipelineLayout, 0, m_frameDescriptor.size(),
                                        m_frameDescriptor.data(), 0, nullptr);



    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)drawSize.width;
    viewport.height = (float)drawSize.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = drawSize.width;
    scissor.extent.height = drawSize.height;

    vkCmdSetScissor(cmd, 0, 1, &scissor);

    GPUDrawPushConstants push_constants;
    //push_constants.worldMatrix = QMatrix4x4();
    push_constants.vertexBuffer = rectangle.vertexBufferAddress;

    vkCmdPushConstants(cmd, m_meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
    vkCmdBindIndexBuffer(cmd, rectangle.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

    vkCmdEndRendering(cmd);



    vkutil::transition_image(cmd,swapchainImage,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vkutil::transition_image(cmd,m_drawImage.image,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    vkutil::copy_image_to_image(cmd,m_drawImage.image,swapchainImage,drawSize,swapSize);

    vkutil::transition_image(cmd,swapchainImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);



    m_window->frameReady();
   // m_window->requestUpdate();


}

void VulkanRenderer::newImage(VideoFrame frame)
{
    vkQueueWaitIdle(m_window->getGraphicsQueue());
    vmaDestroyImage(m_allocator, m_image.image, m_image.allocation);

    m_image=loadImage(frame);

        DescriptorWriter writer;
        writer.write_image(0,m_image.imageView,m_linearSampler,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        vkQueueWaitIdle(m_window->getGraphicsQueue());

    writer.update_set(m_window->getDevice(),m_frameDescriptor[0]);
    frame.clean();

}

void VulkanRenderer::immediate_submit(std::function<void (VkCommandBuffer)> &&function)
{
    VK_CHECK(vkResetFences(m_window->getDevice(), 1, &m_imFence));
    VK_CHECK(vkResetCommandBuffer(m_imCommandBuffer, 0));

    VkCommandBuffer cmd = m_imCommandBuffer;

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(m_window->getGraphicsQueue(), 1, &submit, m_imFence));

    VK_CHECK(vkWaitForFences(m_window->getDevice(), 1, &m_imFence, true, 9999999999));
}

void VulkanRenderer::preInitResources()
{
   // m_window->setDeviceExtensions({VK_KHR_SWAPCHAIN_EXTENSION_NAME,VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME});

}

AllocatedImage VulkanRenderer::loadImage(std::string path)
{
    AllocatedImage newImage {};

    int width, height, nrChannels;

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
    if (data) {
        VkExtent3D imagesize;
        imagesize.width = width;
        imagesize.height = height;
        imagesize.depth = 1;

        newImage = create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,false);

        stbi_image_free(data);
    }else{
        qDebug() << "could not load image with stbi: " << path;
    }
    return newImage;
}

AllocatedImage VulkanRenderer::loadImage(VideoFrame frame)
{
    AllocatedImage newImage {};

    int width, height, nrChannels;

    //unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
    if (frame.frameData) {
        VkExtent3D imagesize;
        imagesize.width = frame.width;
        imagesize.height = frame.height;
        imagesize.depth = 1;

        newImage = create_image(frame.frameData, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,false);

        //stbi_image_free(data);
    }else{
        qDebug() << "could not load image with stbi: ";
    }
    return newImage;
}

AllocatedImage VulkanRenderer::create_image(void *data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
    size_t data_size = size.depth * size.width * size.height * 4;
    AllocatedBuffer uploadbuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(uploadbuffer.info.pMappedData, data, data_size);

    AllocatedImage new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    immediate_submit([&](VkCommandBuffer cmd) {
        vkutil::transition_image(cmd,new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = size;

        // copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &copyRegion);

        if (mipmapped) {
           //generate_mipmaps(cmd, new_image.image,VkExtent2D{new_image.imageExtent.width,new_image.imageExtent.height});
        } else {
            vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    });

    destroy_buffer(uploadbuffer);

    return new_image;
}

AllocatedImage VulkanRenderer::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
    AllocatedImage newImage;
    newImage.imageFormat = format;
    newImage.imageExtent = size;

    VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
    if (mipmapped) {
        img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
    }

    // always allocate images on dedicated GPU memory
    VmaAllocationCreateInfo allocinfo = {};
    allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    VK_CHECK(vmaCreateImage(m_allocator, &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr));

    // if the format is a depth format, we will need to have it use the correct
    // aspect flag
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // build a image-view for the image
    VkImageViewCreateInfo view_info = vkinit::imageview_create_info(format, newImage.image, aspectFlag);
    view_info.subresourceRange.levelCount = img_info.mipLevels;

    VK_CHECK(vkCreateImageView(m_window->getDevice(), &view_info, nullptr, &newImage.imageView));

    return newImage;
}

GPUMeshBuffers VulkanRenderer::uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices){
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers newSurface;

    //create vertex buffer
    newSurface.vertexBuffer = create_buffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                            VMA_MEMORY_USAGE_GPU_ONLY);

    //find the adress of the vertex buffer
    VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(m_window->getDevice(), &deviceAdressInfo);

    //create index buffer
    newSurface.indexBuffer = create_buffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                           VMA_MEMORY_USAGE_GPU_ONLY);

    AllocatedBuffer staging = create_buffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data = staging.allocation->GetMappedData();


    // copy vertex buffer
    memcpy(data, vertices.data(), vertexBufferSize);
    // copy index buffer
    memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

    immediate_submit([&](VkCommandBuffer cmd) {
        VkBufferCopy vertexCopy{ 0 };
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy{ 0 };
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
    });

    destroy_buffer(staging);

    return newSurface;
}

void VulkanRenderer::update()
{
    m_transformData.scale+=0.005;
    *m_transformDataUniform  = m_transformData;
    if(m_transformData.scale> 2)
    {
        // AllocatedImage nextImage = m_image2;
        // if(m_nextImage%2==1)
        //     nextImage=m_image;
        // DescriptorWriter writer;
        // writer.write_image(0,nextImage.imageView,m_linearSampler,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        // vkQueueWaitIdle(m_window->getGraphicsQueue());

        // writer.update_set(m_window->getDevice(),m_frameDescriptor[0]);
        m_transformData.scale = 1;
        *m_transformDataUniform  = m_transformData;
        m_nextImage++;
        qDebug()<<m_nextImage;
    }
    {
        DescriptorWriter writer;
        writer.write_buffer(0, m_transformDataBuffer.buffer, sizeof(TransformData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        vkQueueWaitIdle(m_window->getGraphicsQueue());
        writer.update_set(m_window->getDevice(), m_frameDescriptor[1]);
    }
}


