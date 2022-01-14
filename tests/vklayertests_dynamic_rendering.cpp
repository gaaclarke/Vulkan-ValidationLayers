/*
 * Copyright (c) 2015-2022 The Khronos Group Inc.
 * Copyright (c) 2015-2022 Valve Corporation
 * Copyright (c) 2015-2022 LunarG, Inc.
 * Copyright (c) 2015-2022 Google, Inc.
 * Modifications Copyright (C) 2020-2022 Advanced Micro Devices, Inc. All rights reserved.
 * Modifications Copyright (C) 2021 ARM, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "cast_utils.h"
#include "layer_validation_tests.h"

TEST_F(VkLayerTest, CommandBufferInheritanceRenderingInfoKHR) {
    TEST_DESCRIPTION("VkCommandBufferInheritanceRenderingInfoKHR Dynamic Rendering Tests.");

    uint32_t version = SetTargetApiVersion(VK_API_VERSION_1_2);
    if (version < VK_API_VERSION_1_2) {
        printf("%s At least Vulkan version 1.2 is required, skipping test.\n", kSkipPrefix);
        return;
    }
    
    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    VkPhysicalDeviceFeatures2 features2 = LvlInitStruct<VkPhysicalDeviceFeatures2>();
    ASSERT_NO_FATAL_FAILURE(Init(nullptr, &features2, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT));

    if (DeviceValidationVersion() < VK_API_VERSION_1_2) {
        printf("%s At least Vulkan version 1.2 is required for device, skipping test\n", kSkipPrefix);
        return;
    }
    
    if (!AreRequestedExtensionsEnabled()) {
        printf("%s %s is not supported; skipping\n", kSkipPrefix, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        return;
    }

    VkPhysicalDeviceMultiviewProperties multiview_props = LvlInitStruct<VkPhysicalDeviceMultiviewProperties>();
    VkPhysicalDeviceProperties2 pd_props2 = LvlInitStruct<VkPhysicalDeviceProperties2>(&multiview_props);
    vk::GetPhysicalDeviceProperties2(gpu(), &pd_props2);

    if (multiview_props.maxMultiviewViewCount == 32) {
        printf("%s VUID is not testable as maxMultiviewViewCount is 32, skipping test\n", kSkipPrefix);
        return;
    }

    VkFormat color_format = VK_FORMAT_D32_SFLOAT;

    auto cmd_buffer_inheritance_rendering_info = LvlInitStruct<VkCommandBufferInheritanceRenderingInfoKHR>();
    cmd_buffer_inheritance_rendering_info.colorAttachmentCount = 1;
    cmd_buffer_inheritance_rendering_info.pColorAttachmentFormats = &color_format;
    cmd_buffer_inheritance_rendering_info.depthAttachmentFormat = VK_FORMAT_R8G8B8_UNORM;
    cmd_buffer_inheritance_rendering_info.stencilAttachmentFormat = VK_FORMAT_R8G8B8_SNORM;
    cmd_buffer_inheritance_rendering_info.viewMask = 1 << multiview_props.maxMultiviewViewCount;

    auto sample_count_info_amd = LvlInitStruct<VkAttachmentSampleCountInfoAMD>();
    sample_count_info_amd.pNext = &cmd_buffer_inheritance_rendering_info;
    sample_count_info_amd.colorAttachmentCount = 2;
    
    auto cmd_buffer_inheritance_info = LvlInitStruct<VkCommandBufferInheritanceInfo>();
    cmd_buffer_inheritance_info.pNext = &sample_count_info_amd;

    auto cmd_buffer_allocate_info = LvlInitStruct<VkCommandBufferAllocateInfo>();
    cmd_buffer_allocate_info.commandPool = m_commandPool->handle();
    cmd_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    cmd_buffer_allocate_info.commandBufferCount = 0x1;

    VkCommandBuffer secondary_cmd_buffer;
    VkResult err = vk::AllocateCommandBuffers(m_device->device(), &cmd_buffer_allocate_info, &secondary_cmd_buffer);
    ASSERT_VK_SUCCESS(err);
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkCommandBufferBeginInfo-flags-06003");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkCommandBufferInheritanceRenderingInfoKHR-colorAttachmentCount-06004");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkCommandBufferInheritanceRenderingInfoKHR-variableMultisampleRate-06005");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkCommandBufferInheritanceRenderingInfoKHR-depthAttachmentFormat-06007");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkCommandBufferInheritanceRenderingInfoKHR-multiview-06008");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkCommandBufferInheritanceRenderingInfoKHR-viewMask-06009");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit,
                                         "VUID-VkCommandBufferInheritanceRenderingInfoKHR-stencilAttachmentFormat-06199");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit,
                                         "VUID-VkCommandBufferInheritanceRenderingInfoKHR-depthAttachmentFormat-06200");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit,
                                         "VUID-VkCommandBufferInheritanceRenderingInfoKHR-pColorAttachmentFormats-06006");

    VkCommandBufferBeginInfo cmd_buffer_begin_info = LvlInitStruct<VkCommandBufferBeginInfo>();
    cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    cmd_buffer_begin_info.pInheritanceInfo = &cmd_buffer_inheritance_info;
    vk::BeginCommandBuffer(secondary_cmd_buffer, &cmd_buffer_begin_info);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CommandDrawDynamicRendering) {
    TEST_DESCRIPTION("vkCmdDraw* Dynamic Rendering Tests.");

    uint32_t version = SetTargetApiVersion(VK_API_VERSION_1_2);
    if (version < VK_API_VERSION_1_2) {
        printf("%s At least Vulkan version 1.2 is required, skipping test.\n", kSkipPrefix);
        return;
    }

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));

    if (DeviceValidationVersion() < VK_API_VERSION_1_2) {
        printf("%s At least Vulkan version 1.2 is required for device, skipping test\n", kSkipPrefix);
        return;
    }

    if (!AreRequestedExtensionsEnabled()) {
        printf("%s %s is not supported; skipping\n", kSkipPrefix, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        return;
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = LvlInitStruct<VkPhysicalDeviceFeatures2>(&dynamic_rendering_features);
    vk::GetPhysicalDeviceFeatures2(gpu(), &features2);
    if (!dynamic_rendering_features.dynamicRendering) {
        printf("%s Test requires (unsupported) dynamicRendering , skipping\n", kSkipPrefix);
        return;
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddDefaultColorAttachment();

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});
 
    VkFormat depth_format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();
    pipeline_rendering_info.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
    pipeline_rendering_info.stencilAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

    auto multisample_state_create_info = LvlInitStruct<VkPipelineMultisampleStateCreateInfo>();
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_2_BIT;

    auto create_info = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe.InitGraphicsPipelineCreateInfo(&create_info);
    create_info.pMultisampleState = &multisample_state_create_info;
    create_info.renderPass = VkRenderPass(0x1);
    create_info.pNext = &pipeline_rendering_info;
 
    VkResult err = pipe.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info);
    ASSERT_VK_SUCCESS(err);

    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    VkRect2D scissor = {{0, 0}, {16, 16}}; 

    VkImageObj image(m_device);
    image.Init(32, 32, 1, depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_LINEAR, 0);
    ASSERT_TRUE(image.initialized());

    VkImageViewCreateInfo ivci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                  nullptr,
                                  0,
                                  image.handle(),
                                  VK_IMAGE_VIEW_TYPE_2D,
                                  depth_format,
                                  {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                   VK_COMPONENT_SWIZZLE_IDENTITY},
                                  {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1}};

    VkImageView depth_image_view;
    err = vk::CreateImageView(m_device->device(), &ivci, nullptr, &depth_image_view);
    ASSERT_VK_SUCCESS(err);

    VkRenderingAttachmentInfoKHR depth_attachment = LvlInitStruct<VkRenderingAttachmentInfoKHR>();
    depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth_attachment.imageView = depth_image_view;
 
    VkRenderingInfoKHR begin_rendering_info = LvlInitStruct<VkRenderingInfoKHR>();
    begin_rendering_info.pDepthAttachment = &depth_attachment;
    begin_rendering_info.pStencilAttachment = &depth_attachment;

    m_commandBuffer->begin();
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vk::CmdSetViewport(m_commandBuffer->handle(), 0, 1, &viewport);
    vk::CmdSetScissor(m_commandBuffer->handle(), 0, 1, &scissor);
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-pDepthAttachment-06189");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-pStencilAttachment-06190");
    vk::CmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_errorMonitor->VerifyFound();
    m_commandBuffer->EndRendering();
    m_commandBuffer->end();
}

TEST_F(VkLayerTest, DynamicRenderingGraphicsPipelineCreateInfo) {
    TEST_DESCRIPTION("Test graphics pipeline creation with dynamic rendering.");
    
    uint32_t version = SetTargetApiVersion(VK_API_VERSION_1_2);
    if (version < VK_API_VERSION_1_2) {
        printf("%s At least Vulkan version 1.2 is required, skipping test.\n", kSkipPrefix);
        return;
    }

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));

    if (DeviceValidationVersion() < VK_API_VERSION_1_2) {
        printf("%s At least Vulkan version 1.2 is required for device, skipping test\n", kSkipPrefix);
        return;
    }

     if (!AreRequestedExtensionsEnabled()) {
        printf("%s %s is not supported; skipping\n", kSkipPrefix, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        return;
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = LvlInitStruct<VkPhysicalDeviceFeatures2>(&dynamic_rendering_features);
    vk::GetPhysicalDeviceFeatures2(gpu(), &features2);
    if (!dynamic_rendering_features.dynamicRendering) {
        printf("%s Test requires (unsupported) dynamicRendering , skipping\n", kSkipPrefix);
        return;
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    const VkPipelineLayoutObj pl(m_device);
    VkPipelineObj pipe(m_device);

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};

    auto color_blend_state_create_info = LvlInitStruct<VkPipelineColorBlendStateCreateInfo>();
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;

    VkFormat color_format[2] = {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_D32_SFLOAT_S8_UINT};

    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();
    pipeline_rendering_info.colorAttachmentCount = 2;
    pipeline_rendering_info.pColorAttachmentFormats = &color_format[0];
    pipeline_rendering_info.viewMask = 0x2;

    auto pipeline_tessellation_state_info = LvlInitStruct<VkPipelineTessellationStateCreateInfo>(); 
    pipeline_tessellation_state_info.patchControlPoints = 1;

    auto pipeline_input_assembly_state_info = LvlInitStruct<VkPipelineInputAssemblyStateCreateInfo>();
    pipeline_input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

    auto create_info = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj gs(m_device, bindStateGeomShaderText, VK_SHADER_STAGE_GEOMETRY_BIT, this);
    VkShaderObj te(m_device, bindStateTeshaderText, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, this);
    VkShaderObj tc(m_device, bindStateTscShaderText, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, this);
    pipe.AddShader(&vs);
    pipe.AddShader(&gs);
    pipe.AddShader(&te);
    pipe.AddShader(&tc);
    pipe.InitGraphicsPipelineCreateInfo(&create_info);
    create_info.pColorBlendState = &color_blend_state_create_info;
    create_info.pNext = &pipeline_rendering_info;
    create_info.pTessellationState = &pipeline_tessellation_state_info;
    create_info.pInputAssemblyState = &pipeline_input_assembly_state_info;
    
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkGraphicsPipelineCreateInfo-renderPass-06055");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkGraphicsPipelineCreateInfo-renderPass-06057");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkGraphicsPipelineCreateInfo-renderPass-06058");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkGraphicsPipelineCreateInfo-renderPass-06060");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkGraphicsPipelineCreateInfo-renderPass-06062");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkPipelineRenderingCreateInfoKHR-multiview-06066");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkPipelineRenderingCreateInfoKHR-pColorAttachmentFormats-06064");
    pipe.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info);
    m_errorMonitor->VerifyFound();

}

TEST_F(VkLayerTest, DynamicRenderingWithMismatchingViewMask) {
    TEST_DESCRIPTION("Draw with Dynamic Rendering and a mismatching viewMask");

    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequestedExtensionsEnabled()) {
        printf("%s %s Extension not supported, skipping tests\n", kSkipPrefix, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        return;
    }

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        printf("%s Tests requires Vulkan 1.1+, skipping test\n", kSkipPrefix);
        return;
    }

    auto multiview_features = LvlInitStruct<VkPhysicalDeviceMultiviewFeatures>();
    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>(&multiview_features);
    auto features2 = LvlInitStruct<VkPhysicalDeviceFeatures2>(&dynamic_rendering_features);
    vk::GetPhysicalDeviceFeatures2(gpu(), &features2);
    if (!dynamic_rendering_features.dynamicRendering) {
        printf("%s Test requires (unsupported) dynamicRendering , skipping\n", kSkipPrefix);
        return;
    }
    if (!multiview_features.multiview) {
        printf("%s Test requires (unsupported) multiview , skipping\n", kSkipPrefix);
        return;
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    char const *fsSource = R"glsl(
        #version 450
        layout(location=0) out vec4 color;
        void main() {
           color = vec4(1.0f);
        }
    )glsl";

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddDefaultColorAttachment();

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});

    VkFormat color_formats = {VK_FORMAT_R8G8B8A8_UNORM};
    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &color_formats;
    pipeline_rendering_info.viewMask = 1;

    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    VkRect2D scissor = {{0, 0}, {16, 16}};
    m_viewports.push_back(viewport);
    m_scissors.push_back(scissor);
    pipe.SetViewport(m_viewports);
    pipe.SetScissor(m_scissors);

    auto create_info = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe.InitGraphicsPipelineCreateInfo(&create_info);
    create_info.pNext = &pipeline_rendering_info;

    pipe.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info);

    VkRenderingAttachmentInfoKHR color_attachment = {};
    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkRenderingInfoKHR begin_rendering_info = {};
    begin_rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    begin_rendering_info.colorAttachmentCount = 1;
    begin_rendering_info.pColorAttachments = &color_attachment;
    begin_rendering_info.viewMask = 2;
    begin_rendering_info.layerCount = 1;

    m_commandBuffer->begin();
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-viewMask-06178");
    m_commandBuffer->Draw(1, 1, 0, 0);
    m_errorMonitor->VerifyFound();
    m_commandBuffer->EndRendering();
    m_commandBuffer->end();
}

TEST_F(VkLayerTest, DynamicRenderingWithMistmatchingAttachments) {
    TEST_DESCRIPTION("Draw with Dynamic Rendering with mismatching color attachment counts and depth/stencil formats");

    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequestedExtensionsEnabled()) {
        printf("%s %s Extension not supported, skipping tests\n", kSkipPrefix, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        return;
    }

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        printf("%s Tests requires Vulkan 1.1+, skipping test\n", kSkipPrefix);
        return;
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = LvlInitStruct<VkPhysicalDeviceFeatures2>(&dynamic_rendering_features);
    vk::GetPhysicalDeviceFeatures2(gpu(), &features2);
    if (!dynamic_rendering_features.dynamicRendering) {
        printf("%s Test requires (unsupported) dynamicRendering , skipping\n", kSkipPrefix);
        return;
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    char const *fsSource = R"glsl(
        #version 450
        layout(location=0) out vec4 color;
        void main() {
           color = vec4(1.0f);
        }
    )glsl";

    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    VkRect2D scissor = {{0, 0}, {16, 16}};
    m_viewports.push_back(viewport);
    m_scissors.push_back(scissor);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});

    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();

    VkPipelineObj pipe1(m_device);
    pipe1.AddShader(&vs);
    pipe1.AddShader(&fs);
    pipe1.AddDefaultColorAttachment();
    pipe1.SetViewport(m_viewports);
    pipe1.SetScissor(m_scissors);

    VkFormat color_formats[] = {VK_FORMAT_R8G8B8A8_UNORM};
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = color_formats;

    auto create_info1 = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe1.InitGraphicsPipelineCreateInfo(&create_info1);
    create_info1.pNext = &pipeline_rendering_info;

    pipe1.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info1);

    VkPipelineObj pipe2(m_device);
    pipe2.AddShader(&vs);
    pipe2.AddShader(&fs);
    pipe2.AddDefaultColorAttachment();
    pipe2.SetViewport(m_viewports);
    pipe2.SetScissor(m_scissors);

    pipeline_rendering_info.colorAttachmentCount = 0;
    pipeline_rendering_info.pColorAttachmentFormats = nullptr;
    pipeline_rendering_info.depthAttachmentFormat = VK_FORMAT_D16_UNORM;

    auto create_info2 = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe2.InitGraphicsPipelineCreateInfo(&create_info2);
    create_info2.pNext = &pipeline_rendering_info;

    pipe2.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info2);

    VkFormat depthStencilFormat = FindSupportedDepthStencilFormat(gpu());
    ASSERT_TRUE(depthStencilFormat != 0);

    bool testStencil = false;
    VkFormat stencilFormat = VK_FORMAT_UNDEFINED;

    if (ImageFormatIsSupported(gpu(), VK_FORMAT_S8_UINT, VK_IMAGE_TILING_OPTIMAL)) {
        stencilFormat = VK_FORMAT_S8_UINT;
        testStencil = true;
    } else if ((depthStencilFormat != VK_FORMAT_D16_UNORM_S8_UINT) &&
               ImageFormatIsSupported(gpu(), VK_FORMAT_D16_UNORM_S8_UINT, VK_IMAGE_TILING_OPTIMAL)) {
        stencilFormat = VK_FORMAT_D16_UNORM_S8_UINT;
        testStencil = true;
    } else if ((depthStencilFormat != VK_FORMAT_D24_UNORM_S8_UINT) &&
               ImageFormatIsSupported(gpu(), VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_TILING_OPTIMAL)) {
        stencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
        testStencil = true;
    } else if ((depthStencilFormat != VK_FORMAT_D32_SFLOAT_S8_UINT) &&
               ImageFormatIsSupported(gpu(), VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_TILING_OPTIMAL)) {
        stencilFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
        testStencil = true;
    }

    VkPipelineObj pipe3(m_device);

    if (testStencil) {
        pipe3.AddShader(&vs);
        pipe3.AddShader(&fs);
        pipe3.AddDefaultColorAttachment();
        pipe3.SetViewport(m_viewports);
        pipe3.SetScissor(m_scissors);

        pipeline_rendering_info.colorAttachmentCount = 0;
        pipeline_rendering_info.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
        pipeline_rendering_info.stencilAttachmentFormat = stencilFormat;

        auto create_info3 = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
        pipe3.InitGraphicsPipelineCreateInfo(&create_info3);
        create_info3.pNext = &pipeline_rendering_info;

        pipe3.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info3);
    }

    VkImageObj colorImage(m_device);
    colorImage.Init(32, 32, 1, VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    VkImageView colorImageView = colorImage.targetView(VK_FORMAT_R8G8B8A8_UINT);

    VkImageObj depthStencilImage(m_device);
    depthStencilImage.Init(32, 32, 1, depthStencilFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    VkImageView depthStencilImageView =
        depthStencilImage.targetView(depthStencilFormat, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

    VkRenderingAttachmentInfoKHR color_attachment = {};
    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.imageView = colorImageView;

    VkRenderingAttachmentInfoKHR depth_stencil_attachment = {};
    depth_stencil_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    depth_stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_stencil_attachment.imageView = depthStencilImageView;

    VkRenderingInfoKHR begin_rendering_info = {};
    begin_rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;

    m_commandBuffer->begin();

    // Mismatching color attachment count
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe1.handle());
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-colorAttachmentCount-06179");
    m_commandBuffer->Draw(1, 1, 0, 0);
    m_errorMonitor->VerifyFound();
    m_commandBuffer->EndRendering();

    // Mismatching color formats
    begin_rendering_info.colorAttachmentCount = 1;
    begin_rendering_info.pColorAttachments = &color_attachment;
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe1.handle());
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-colorAttachmentCount-06180");
    m_commandBuffer->Draw(1, 1, 0, 0);
    m_errorMonitor->VerifyFound();
    m_commandBuffer->EndRendering();

    // Mismatching depth format
    begin_rendering_info.colorAttachmentCount = 0;
    begin_rendering_info.pColorAttachments = nullptr;
    begin_rendering_info.pDepthAttachment = &depth_stencil_attachment;
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe2.handle());
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-pDepthAttachment-06181");
    m_commandBuffer->Draw(1, 1, 0, 0);
    m_errorMonitor->VerifyFound();
    m_commandBuffer->EndRendering();

    // Mismatching stencil format
    if (testStencil) {
        begin_rendering_info.pDepthAttachment = nullptr;
        begin_rendering_info.pStencilAttachment = &depth_stencil_attachment;
        m_commandBuffer->BeginRendering(begin_rendering_info);
        vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe3.handle());
        m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-pStencilAttachment-06182");
        m_commandBuffer->Draw(1, 1, 0, 0);
        m_errorMonitor->VerifyFound();
        m_commandBuffer->EndRendering();
    }

    m_commandBuffer->end();
}

TEST_F(VkLayerTest, DynamicRenderingWithMistmatchingAttachmentSamples) {
    TEST_DESCRIPTION("Draw with Dynamic Rendering with mismatching color/depth/stencil sample counts");

    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequestedExtensionsEnabled()) {
        printf("%s %s Extension not supported, skipping tests\n", kSkipPrefix, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        return;
    }

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        printf("%s Tests requires Vulkan 1.1+, skipping test\n", kSkipPrefix);
        return;
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = LvlInitStruct<VkPhysicalDeviceFeatures2>(&dynamic_rendering_features);
    vk::GetPhysicalDeviceFeatures2(gpu(), &features2);
    if (!dynamic_rendering_features.dynamicRendering) {
        printf("%s Test requires (unsupported) dynamicRendering , skipping\n", kSkipPrefix);
        return;
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    char const *fsSource = R"glsl(
        #version 450
        layout(location=0) out vec4 color;
        void main() {
           color = vec4(1.0f);
        }
    )glsl";

    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    VkRect2D scissor = {{0, 0}, {16, 16}};
    m_viewports.push_back(viewport);
    m_scissors.push_back(scissor);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});

    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();


    VkPipelineObj pipe1(m_device);
    pipe1.AddShader(&vs);
    pipe1.AddShader(&fs);
    pipe1.AddDefaultColorAttachment();
    pipe1.SetViewport(m_viewports);
    pipe1.SetScissor(m_scissors);

    VkFormat color_formats[] = {VK_FORMAT_R8G8B8A8_UNORM};
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = color_formats;

    auto create_info1 = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe1.InitGraphicsPipelineCreateInfo(&create_info1);
    create_info1.pNext = &pipeline_rendering_info;

    auto multisample_state_create_info = LvlInitStruct<VkPipelineMultisampleStateCreateInfo>();
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_2_BIT;
    create_info1.pMultisampleState = &multisample_state_create_info;

    pipe1.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info1);


    VkFormat depthStencilFormat = FindSupportedDepthStencilFormat(gpu());
    ASSERT_TRUE(depthStencilFormat != 0);


    VkPipelineObj pipe2(m_device);
    pipe2.AddShader(&vs);
    pipe2.AddShader(&fs);
    pipe2.AddDefaultColorAttachment();
    pipe2.SetViewport(m_viewports);
    pipe2.SetScissor(m_scissors);

    pipeline_rendering_info.colorAttachmentCount = 0;
    pipeline_rendering_info.pColorAttachmentFormats = nullptr;
    pipeline_rendering_info.depthAttachmentFormat = depthStencilFormat;

    auto create_info2 = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe2.InitGraphicsPipelineCreateInfo(&create_info2);
    create_info2.pNext = &pipeline_rendering_info;

    create_info2.pMultisampleState = &multisample_state_create_info;

    pipe2.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info2);


    VkPipelineObj pipe3(m_device);

    pipe3.AddShader(&vs);
    pipe3.AddShader(&fs);
    pipe3.AddDefaultColorAttachment();
    pipe3.SetViewport(m_viewports);
    pipe3.SetScissor(m_scissors);

    pipeline_rendering_info.colorAttachmentCount = 0;
    pipeline_rendering_info.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    pipeline_rendering_info.stencilAttachmentFormat = depthStencilFormat;

    auto create_info3 = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe3.InitGraphicsPipelineCreateInfo(&create_info3);
    create_info3.pNext = &pipeline_rendering_info;

    create_info3.pMultisampleState = &multisample_state_create_info;

    pipe3.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info3);


    VkImageObj colorImage(m_device);
    colorImage.Init(32, 32, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    VkImageView colorImageView = colorImage.targetView(VK_FORMAT_R8G8B8A8_UNORM);

    VkImageObj depthStencilImage(m_device);
    depthStencilImage.Init(32, 32, 1, depthStencilFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    VkImageView depthStencilImageView =
        depthStencilImage.targetView(depthStencilFormat, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

    VkRenderingAttachmentInfoKHR color_attachment = {};
    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.imageView = colorImageView;

    VkRenderingAttachmentInfoKHR depth_stencil_attachment = {};
    depth_stencil_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    depth_stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_stencil_attachment.imageView = depthStencilImageView;

    VkRenderingInfoKHR begin_rendering_info = {};
    begin_rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;

    m_commandBuffer->begin();

    // Mismatching color samples
    begin_rendering_info.colorAttachmentCount = 1;
    begin_rendering_info.pColorAttachments = &color_attachment;
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe1.handle());
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-colorAttachmentCount-06188");
    m_commandBuffer->Draw(1, 1, 0, 0);
    m_errorMonitor->VerifyFound();
    m_commandBuffer->EndRendering();

    // Mismatching depth samples
    begin_rendering_info.colorAttachmentCount = 0;
    begin_rendering_info.pColorAttachments = nullptr;
    begin_rendering_info.pDepthAttachment = &depth_stencil_attachment;
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe2.handle());
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-pDepthAttachment-06189");
    m_commandBuffer->Draw(1, 1, 0, 0);
    m_errorMonitor->VerifyFound();
    m_commandBuffer->EndRendering();

    // Mismatching stencil samples
    begin_rendering_info.pDepthAttachment = nullptr;
    begin_rendering_info.pStencilAttachment = &depth_stencil_attachment;
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe3.handle());
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-pStencilAttachment-06190");
    m_commandBuffer->Draw(1, 1, 0, 0);
    m_errorMonitor->VerifyFound();
    m_commandBuffer->EndRendering();

    m_commandBuffer->end();
}

TEST_F(VkLayerTest, DynamicRenderingWithMistmatchingMixedAttachmentSamples) {
    TEST_DESCRIPTION("Draw with Dynamic Rendering with mismatching mixed color/depth/stencil sample counts");

    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequestedExtensionsEnabled()) {
        printf("%s %s Extension not supported, skipping tests\n", kSkipPrefix, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        return;
    }

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        printf("%s Tests requires Vulkan 1.1+, skipping test\n", kSkipPrefix);
        return;
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = LvlInitStruct<VkPhysicalDeviceFeatures2>(&dynamic_rendering_features);
    vk::GetPhysicalDeviceFeatures2(gpu(), &features2);
    if (!dynamic_rendering_features.dynamicRendering) {
        printf("%s Test requires (unsupported) dynamicRendering , skipping\n", kSkipPrefix);
        return;
    }

    bool amd_samples = false;
    if (DeviceExtensionSupported(gpu(), nullptr, VK_AMD_MIXED_ATTACHMENT_SAMPLES_EXTENSION_NAME)) {
        m_device_extension_names.push_back(VK_AMD_MIXED_ATTACHMENT_SAMPLES_EXTENSION_NAME);
        amd_samples = true;
    }

    bool nv_samples = false;
    if (DeviceExtensionSupported(gpu(), nullptr, VK_NV_FRAMEBUFFER_MIXED_SAMPLES_EXTENSION_NAME)) {
        m_device_extension_names.push_back(VK_NV_FRAMEBUFFER_MIXED_SAMPLES_EXTENSION_NAME);
        nv_samples = true;
    }

    if (!amd_samples && !nv_samples) {
        printf("%s Test requires either VK_AMD_mixed_attachment_samples or VK_NV_framebuffer_mixed_samples, skipping\n",
               kSkipPrefix);
        return;
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    char const *fsSource = R"glsl(
        #version 450
        layout(location=0) out vec4 color;
        void main() {
           color = vec4(1.0f);
        }
    )glsl";

    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    VkRect2D scissor = {{0, 0}, {16, 16}};
    m_viewports.push_back(viewport);
    m_scissors.push_back(scissor);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});

    VkSampleCountFlagBits counts = {VK_SAMPLE_COUNT_2_BIT};
    auto samples_info = LvlInitStruct<VkAttachmentSampleCountInfoAMD>();

    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>(&samples_info);

    VkPipelineObj pipe1(m_device);
    pipe1.AddShader(&vs);
    pipe1.AddShader(&fs);
    pipe1.AddDefaultColorAttachment();
    pipe1.SetViewport(m_viewports);
    pipe1.SetScissor(m_scissors);

    VkFormat color_formats[] = {VK_FORMAT_R8G8B8A8_UNORM};
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = color_formats;

    samples_info.colorAttachmentCount = 1;
    samples_info.pColorAttachmentSamples = &counts;

    auto create_info1 = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe1.InitGraphicsPipelineCreateInfo(&create_info1);
    create_info1.pNext = &pipeline_rendering_info;

    pipe1.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info1);

    VkFormat depthStencilFormat = FindSupportedDepthStencilFormat(gpu());
    ASSERT_TRUE(depthStencilFormat != 0);

    VkPipelineObj pipe2(m_device);
    pipe2.AddShader(&vs);
    pipe2.AddShader(&fs);
    pipe2.AddDefaultColorAttachment();
    pipe2.SetViewport(m_viewports);
    pipe2.SetScissor(m_scissors);

    pipeline_rendering_info.colorAttachmentCount = 0;
    pipeline_rendering_info.pColorAttachmentFormats = nullptr;
    pipeline_rendering_info.depthAttachmentFormat = depthStencilFormat;

    samples_info.colorAttachmentCount = 0;
    samples_info.pColorAttachmentSamples = nullptr;
    samples_info.depthStencilAttachmentSamples = counts;

    auto create_info2 = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe2.InitGraphicsPipelineCreateInfo(&create_info2);
    create_info2.pNext = &pipeline_rendering_info;

    pipe2.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info2);

    VkPipelineObj pipe3(m_device);

    pipe3.AddShader(&vs);
    pipe3.AddShader(&fs);
    pipe3.AddDefaultColorAttachment();
    pipe3.SetViewport(m_viewports);
    pipe3.SetScissor(m_scissors);

    pipeline_rendering_info.colorAttachmentCount = 0;
    pipeline_rendering_info.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    pipeline_rendering_info.stencilAttachmentFormat = depthStencilFormat;

    auto create_info3 = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe3.InitGraphicsPipelineCreateInfo(&create_info3);
    create_info3.pNext = &pipeline_rendering_info;

    pipe3.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info3);

    VkImageObj colorImage(m_device);
    colorImage.Init(32, 32, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    VkImageView colorImageView = colorImage.targetView(VK_FORMAT_R8G8B8A8_UNORM);

    VkImageObj depthStencilImage(m_device);
    depthStencilImage.Init(32, 32, 1, depthStencilFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    VkImageView depthStencilImageView =
        depthStencilImage.targetView(depthStencilFormat, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

    VkRenderingAttachmentInfoKHR color_attachment = {};
    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.imageView = colorImageView;

    VkRenderingAttachmentInfoKHR depth_stencil_attachment = {};
    depth_stencil_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    depth_stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_stencil_attachment.imageView = depthStencilImageView;

    VkRenderingInfoKHR begin_rendering_info = {};
    begin_rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;

    m_commandBuffer->begin();

    // Mismatching color samples
    begin_rendering_info.colorAttachmentCount = 1;
    begin_rendering_info.pColorAttachments = &color_attachment;
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe1.handle());
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-colorAttachmentCount-06185");
    m_commandBuffer->Draw(1, 1, 0, 0);
    m_errorMonitor->VerifyFound();
    m_commandBuffer->EndRendering();

    // Mismatching depth samples
    begin_rendering_info.colorAttachmentCount = 0;
    begin_rendering_info.pColorAttachments = nullptr;
    begin_rendering_info.pDepthAttachment = &depth_stencil_attachment;
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe2.handle());
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-pDepthAttachment-06186");
    m_commandBuffer->Draw(1, 1, 0, 0);
    m_errorMonitor->VerifyFound();
    m_commandBuffer->EndRendering();

    // Mismatching stencil samples
    begin_rendering_info.pDepthAttachment = nullptr;
    begin_rendering_info.pStencilAttachment = &depth_stencil_attachment;
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe3.handle());
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdDraw-pStencilAttachment-06187");
    m_commandBuffer->Draw(1, 1, 0, 0);
    m_errorMonitor->VerifyFound();
    m_commandBuffer->EndRendering();

    m_commandBuffer->end();
}