/*
 * Copyright (c) 2015-2023 The Khronos Group Inc.
 * Copyright (c) 2015-2023 Valve Corporation
 * Copyright (c) 2015-2023 LunarG, Inc.
 * Copyright (c) 2015-2023 Google, Inc.
 * Modifications Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "../framework/layer_validation_tests.h"

class NegativeShaderPushConstants : public VkLayerTest {};

TEST_F(NegativeShaderPushConstants, NotDeclared) {
    TEST_DESCRIPTION(
        "Create a graphics pipeline in which a push constant range containing a push constant block member is not declared in the "
        "layout.");

    ASSERT_NO_FATAL_FAILURE(Init());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = R"glsl(
        #version 450
        layout(push_constant, std430) uniform foo { float x; } consts;
        void main(){
           gl_Position = vec4(consts.x);
        }
    )glsl";

    VkShaderObj vs(this, vsSource, VK_SHADER_STAGE_VERTEX_BIT);

    // Set up a push constant range
    VkPushConstantRange push_constant_range = {};
    // Set to the wrong stage to challenge core_validation
    push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    push_constant_range.size = 4;

    const VkPipelineLayoutObj pipeline_layout(m_device, {}, {push_constant_range});

    CreatePipelineHelper pipe(*this);
    pipe.InitInfo();
    pipe.shader_stages_ = {vs.GetStageCreateInfo(), pipe.fs_->GetStageCreateInfo()};
    pipe.InitState();
    pipe.pipeline_layout_ = VkPipelineLayoutObj(m_device, {}, {push_constant_range});

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkGraphicsPipelineCreateInfo-layout-07987");
    pipe.CreateGraphicsPipeline();
    m_errorMonitor->VerifyFound();
}

TEST_F(NegativeShaderPushConstants, PipelineRange) {
    TEST_DESCRIPTION("Invalid use of VkPushConstantRange structs.");

    ASSERT_NO_FATAL_FAILURE(Init());

    VkPhysicalDeviceProperties device_props = {};
    vk::GetPhysicalDeviceProperties(gpu(), &device_props);
    // will be at least 256 as required from the spec
    const uint32_t maxPushConstantsSize = device_props.limits.maxPushConstantsSize;

    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPushConstantRange push_constant_range = {0, 0, 4};
    VkPipelineLayoutCreateInfo pipeline_layout_info{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 0, nullptr, 1, &push_constant_range};

    // stageFlags of 0
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkPushConstantRange-stageFlags-requiredbitmask");
    vk::CreatePipelineLayout(m_device->device(), &pipeline_layout_info, NULL, &pipeline_layout);
    m_errorMonitor->VerifyFound();

    // offset over limit
    push_constant_range = {VK_SHADER_STAGE_VERTEX_BIT, maxPushConstantsSize, 8};
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkPushConstantRange-offset-00294");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkPushConstantRange-size-00298");
    vk::CreatePipelineLayout(m_device->device(), &pipeline_layout_info, NULL, &pipeline_layout);
    m_errorMonitor->VerifyFound();

    // offset not multiple of 4
    push_constant_range = {VK_SHADER_STAGE_VERTEX_BIT, 1, 8};
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkPushConstantRange-offset-00295");
    vk::CreatePipelineLayout(m_device->device(), &pipeline_layout_info, NULL, &pipeline_layout);
    m_errorMonitor->VerifyFound();

    // size of 0
    push_constant_range = {VK_SHADER_STAGE_VERTEX_BIT, 0, 0};
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkPushConstantRange-size-00296");
    vk::CreatePipelineLayout(m_device->device(), &pipeline_layout_info, NULL, &pipeline_layout);
    m_errorMonitor->VerifyFound();

    // size not multiple of 4
    push_constant_range = {VK_SHADER_STAGE_VERTEX_BIT, 0, 7};
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkPushConstantRange-size-00297");
    vk::CreatePipelineLayout(m_device->device(), &pipeline_layout_info, NULL, &pipeline_layout);
    m_errorMonitor->VerifyFound();

    // size over limit
    push_constant_range = {VK_SHADER_STAGE_VERTEX_BIT, 0, maxPushConstantsSize + 4};
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkPushConstantRange-size-00298");
    vk::CreatePipelineLayout(m_device->device(), &pipeline_layout_info, NULL, &pipeline_layout);
    m_errorMonitor->VerifyFound();

    // size over limit of non-zero offset
    push_constant_range = {VK_SHADER_STAGE_VERTEX_BIT, 4, maxPushConstantsSize};
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkPushConstantRange-size-00298");
    vk::CreatePipelineLayout(m_device->device(), &pipeline_layout_info, NULL, &pipeline_layout);
    m_errorMonitor->VerifyFound();

    // Sanity check its a valid range before making duplicate
    push_constant_range = {VK_SHADER_STAGE_VERTEX_BIT, 0, maxPushConstantsSize};
    ASSERT_VK_SUCCESS(vk::CreatePipelineLayout(m_device->device(), &pipeline_layout_info, NULL, &pipeline_layout));
    vk::DestroyPipelineLayout(m_device->device(), pipeline_layout, nullptr);

    // Duplicate ranges
    VkPushConstantRange push_constant_range_duplicate[2] = {push_constant_range, push_constant_range};
    pipeline_layout_info.pushConstantRangeCount = 2;
    pipeline_layout_info.pPushConstantRanges = push_constant_range_duplicate;
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkPipelineLayoutCreateInfo-pPushConstantRanges-00292");
    vk::CreatePipelineLayout(m_device->device(), &pipeline_layout_info, nullptr, &pipeline_layout);
    m_errorMonitor->VerifyFound();
}

TEST_F(NegativeShaderPushConstants, NotInLayout) {
    TEST_DESCRIPTION(
        "Test that an error is produced for a shader consuming push constants which are not provided in the pipeline layout");

    ASSERT_NO_FATAL_FAILURE(Init());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = R"glsl(
        #version 450
        layout(push_constant, std430) uniform foo { float x; } consts;
        void main(){
           gl_Position = vec4(consts.x);
        }
    )glsl";

    VkShaderObj vs(this, vsSource, VK_SHADER_STAGE_VERTEX_BIT);

    CreatePipelineHelper pipe(*this);
    pipe.InitInfo();
    pipe.shader_stages_ = {vs.GetStageCreateInfo(), pipe.fs_->GetStageCreateInfo()};
    pipe.InitState();
    pipe.pipeline_layout_ = VkPipelineLayoutObj(m_device, {});
    /* should have generated an error -- no push constant ranges provided! */
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkGraphicsPipelineCreateInfo-layout-07987");
    pipe.CreateGraphicsPipeline();
    m_errorMonitor->VerifyFound();
}

TEST_F(NegativeShaderPushConstants, Range) {
    TEST_DESCRIPTION("Invalid use of VkPushConstantRange values in vkCmdPushConstants.");

    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));

    PFN_vkSetPhysicalDeviceLimitsEXT fpvkSetPhysicalDeviceLimitsEXT = nullptr;
    PFN_vkGetOriginalPhysicalDeviceLimitsEXT fpvkGetOriginalPhysicalDeviceLimitsEXT = nullptr;
    if (!LoadDeviceProfileLayer(fpvkSetPhysicalDeviceLimitsEXT, fpvkGetOriginalPhysicalDeviceLimitsEXT)) {
        GTEST_SKIP() << "Failed to load device profile layer.";
    }

    // Set limit to be same max as the shader usages
    const uint32_t maxPushConstantsSize = 16;
    VkPhysicalDeviceProperties props;
    fpvkGetOriginalPhysicalDeviceLimitsEXT(gpu(), &props.limits);
    props.limits.maxPushConstantsSize = maxPushConstantsSize;
    fpvkSetPhysicalDeviceLimitsEXT(gpu(), &props.limits);

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *const vsSource = R"glsl(
        #version 450
        layout(push_constant, std430) uniform foo { float x[4]; } constants;
        void main(){
           gl_Position = vec4(constants.x[0]);
        }
    )glsl";

    VkShaderObj const vs(this, vsSource, VK_SHADER_STAGE_VERTEX_BIT);
    VkShaderObj const fs(this, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT);

    // Set up a push constant range
    VkPushConstantRange push_constant_range = {VK_SHADER_STAGE_VERTEX_BIT, 0, maxPushConstantsSize};
    const VkPipelineLayoutObj pipeline_layout(m_device, {}, {push_constant_range});

    CreatePipelineHelper pipe(*this);
    pipe.InitInfo();
    pipe.shader_stages_ = {vs.GetStageCreateInfo(), fs.GetStageCreateInfo()};
    pipe.InitState();
    pipe.pipeline_layout_ = VkPipelineLayoutObj(m_device, {}, {push_constant_range});
    pipe.CreateGraphicsPipeline();

    const float data[16] = {};  // dummy data to match shader size

    m_commandBuffer->begin();

    // size of 0
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdPushConstants-size-arraylength");
    vk::CmdPushConstants(m_commandBuffer->handle(), pipe.pipeline_layout_.handle(), VK_SHADER_STAGE_VERTEX_BIT, 0, 0, data);
    m_errorMonitor->VerifyFound();

    // offset not multiple of 4
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdPushConstants-offset-00368");
    vk::CmdPushConstants(m_commandBuffer->handle(), pipe.pipeline_layout_.handle(), VK_SHADER_STAGE_VERTEX_BIT, 1, 4, data);
    m_errorMonitor->VerifyFound();

    // size not multiple of 4
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdPushConstants-size-00369");
    vk::CmdPushConstants(m_commandBuffer->handle(), pipe.pipeline_layout_.handle(), VK_SHADER_STAGE_VERTEX_BIT, 0, 5, data);
    m_errorMonitor->VerifyFound();

    // offset at limit
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdPushConstants-offset-00370");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdPushConstants-size-00371");
    vk::CmdPushConstants(m_commandBuffer->handle(), pipe.pipeline_layout_.handle(), VK_SHADER_STAGE_VERTEX_BIT,
                         maxPushConstantsSize, 4, data);
    m_errorMonitor->VerifyFound();

    // size at limit
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkCmdPushConstants-size-00371");
    vk::CmdPushConstants(m_commandBuffer->handle(), pipe.pipeline_layout_.handle(), VK_SHADER_STAGE_VERTEX_BIT, 0,
                         maxPushConstantsSize + 4, data);
    m_errorMonitor->VerifyFound();

    // Size at limit, should be valid
    vk::CmdPushConstants(m_commandBuffer->handle(), pipe.pipeline_layout_.handle(), VK_SHADER_STAGE_VERTEX_BIT, 0,
                         maxPushConstantsSize, data);

    m_commandBuffer->end();
}

TEST_F(NegativeShaderPushConstants, DrawWithoutUpdate) {
    TEST_DESCRIPTION("Not every bytes in used push constant ranges has been set before Draw ");

    ASSERT_NO_FATAL_FAILURE(Init());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // push constant range: 0-99
    char const *const vsSource = R"glsl(
        #version 450
        layout(push_constant, std430) uniform foo {
           bool b;
           float f2[3];
           vec3 v;
           vec4 v2[2];
           mat3 m;
        } constants;
        void func1( float f ){
           // use the whole v2[1]. byte: 48-63.
           vec2 v2 = constants.v2[1].yz;
        }
        void main(){
            // use only v2[0].z. byte: 40-43.
            func1( constants.v2[0].z);
            // index of m is variable. The all m is used. byte: 64-99.
            for(int i=1;i<2;++i) {
                vec3 v3 = constants.m[i];
            }
        }
    )glsl";

    // push constant range: 0 - 95
    char const *const fsSource = R"glsl(
        #version 450
        struct foo1{
           int i[4];
        }f;
        layout(push_constant, std430) uniform foo {
           float x[2][2][2];
           foo1 s;
           foo1 ss[3];
        } constants;
        void main(){
            // use s. byte: 32-47.
            f = constants.s;
            // use every i[3] in ss. byte: 60-63, 76-79, 92-95.
            for(int i=1;i<2;++i) {
                int ii = constants.ss[i].i[3];
            }
        }
    )glsl";

    VkShaderObj const vs(this, vsSource, VK_SHADER_STAGE_VERTEX_BIT);
    VkShaderObj const fs(this, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPushConstantRange push_constant_range = {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 128};
    VkPushConstantRange push_constant_range_small = {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 4, 4};

    auto pipeline_layout_info = LvlInitStruct<VkPipelineLayoutCreateInfo>();
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    vk_testing::PipelineLayout pipeline_layout(*m_device, pipeline_layout_info);

    CreatePipelineHelper g_pipe(*this);
    g_pipe.InitInfo();
    g_pipe.shader_stages_ = {vs.GetStageCreateInfo(), fs.GetStageCreateInfo()};
    g_pipe.pipeline_layout_ci_ = pipeline_layout_info;
    g_pipe.InitState();
    ASSERT_VK_SUCCESS(g_pipe.CreateGraphicsPipeline());

    pipeline_layout_info.pPushConstantRanges = &push_constant_range_small;
    vk_testing::PipelineLayout pipeline_layout_small(*m_device, pipeline_layout_info);

    CreatePipelineHelper g_pipe_small_range(*this);
    g_pipe_small_range.InitInfo();
    g_pipe_small_range.shader_stages_ = {vs.GetStageCreateInfo(), fs.GetStageCreateInfo()};
    g_pipe_small_range.pipeline_layout_ci_ = pipeline_layout_info;
    g_pipe_small_range.InitState();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "VUID-VkGraphicsPipelineCreateInfo-layout-07987");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "VUID-VkGraphicsPipelineCreateInfo-layout-07987");
    g_pipe_small_range.CreateGraphicsPipeline();
    m_errorMonitor->VerifyFound();

    m_commandBuffer->begin();
    m_commandBuffer->BeginRenderPass(m_renderPassBeginInfo);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "VUID-vkCmdDraw-maintenance4-06425");  // vertex
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "VUID-vkCmdDraw-maintenance4-06425");  // fragment
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipe.pipeline_);
    vk::CmdBindDescriptorSets(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipe.pipeline_layout_.handle(), 0, 1,
                              &g_pipe.descriptor_set_->set_, 0, nullptr);
    vk::CmdDraw(m_commandBuffer->handle(), 1, 0, 0, 0);
    m_errorMonitor->VerifyFound();

    const float dummy_values[128] = {};

    // NOTE: these are commented out due to ambiguity around VUID 02698 and push constant lifetimes
    //       See https://gitlab.khronos.org/vulkan/vulkan/-/issues/2602 and
    //       https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/2689
    //       for more details.
    // m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "VUID-vkCmdDraw-maintenance4-06425");
    // vk::CmdPushConstants(m_commandBuffer->handle(), g_pipe.pipeline_layout_.handle(),
    //                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 96, dummy_values);
    // vk::CmdDraw(m_commandBuffer->handle(), 1, 0, 0, 0);
    // m_errorMonitor->VerifyFound();

    // m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "VUID-vkCmdDraw-maintenance4-06425");
    // vk::CmdPushConstants(m_commandBuffer->handle(), pipeline_layout_small, VK_SHADER_STAGE_VERTEX_BIT, 4, 4, dummy_values);
    // vk::CmdDraw(m_commandBuffer->handle(), 1, 0, 0, 0);
    // m_errorMonitor->VerifyFound();

    vk::CmdPushConstants(m_commandBuffer->handle(), pipeline_layout.handle(),
                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 32, 68, dummy_values);
    vk::CmdDraw(m_commandBuffer->handle(), 1, 0, 0, 0);

    m_commandBuffer->EndRenderPass();
    m_commandBuffer->end();
}

TEST_F(NegativeShaderPushConstants, MultipleEntryPoint) {
    TEST_DESCRIPTION("Test push-constant detect the write entrypoint with the push constants.");

    ASSERT_NO_FATAL_FAILURE(Init());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    if (IsPlatform(kPixel3) || IsPlatform(kPixel3aXL)) {
        GTEST_SKIP() << "Pixel 3 compilers can't compile this valid SPIR-V";
    }

    // #version 460
    // layout(push_constant) uniform Material {
    //     vec4 color;
    // }constants;
    // void main() {
    //     gl_Position = constants.color;
    // }
    //
    // #version 460
    // layout(location = 0) out vec4 uFragColor;
    // void main(){
    //     uFragColor = vec4(0.0);
    // }
    const char *source = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %main_f "main" %4
               OpEntryPoint Vertex %main_v "main" %2
               OpExecutionMode %main_f OriginUpperLeft
               OpMemberDecorate %builtin_vert 0 BuiltIn Position
               OpMemberDecorate %builtin_vert 1 BuiltIn PointSize
               OpMemberDecorate %builtin_vert 2 BuiltIn ClipDistance
               OpMemberDecorate %builtin_vert 3 BuiltIn CullDistance
               OpDecorate %builtin_vert Block
               OpMemberDecorate %struct_pc 0 Offset 0
               OpDecorate %struct_pc Block
               OpDecorate %4 Location 0
       %void = OpTypeVoid
          %8 = OpTypeFunction %void
      %float = OpTypeFloat 32
            ; Vertex types
    %v4float = OpTypeVector %float 4
       %uint = OpTypeInt 32 0
     %uint_1 = OpConstant %uint 1
      %array = OpTypeArray %float %uint_1
  %builtin_vert = OpTypeStruct %v4float %float %array %array
%ptr_builtin_vert = OpTypePointer Output %builtin_vert
          %2 = OpVariable %ptr_builtin_vert Output
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0
  %struct_pc = OpTypeStruct %v4float
%ptr_pc_struct = OpTypePointer PushConstant %struct_pc
         %18 = OpVariable %ptr_pc_struct PushConstant
%ptr_pc_vec4 = OpTypePointer PushConstant %v4float
%ptr_output_vert = OpTypePointer Output %v4float
            ; Fragment types
%ptr_output_frag = OpTypePointer Output %v4float
          %4 = OpVariable %ptr_output_frag Output
    %float_0 = OpConstant %float 0
         %23 = OpConstantComposite %v4float %float_0 %float_0 %float_0 %float_0

     %main_v = OpFunction %void None %8
         %24 = OpLabel
         %25 = OpAccessChain %ptr_pc_vec4 %18 %int_0
         %26 = OpLoad %v4float %25
         %27 = OpAccessChain %ptr_output_vert %2 %int_0
               OpStore %27 %26
               OpReturn
               OpFunctionEnd

     %main_f = OpFunction %void None %8
         %28 = OpLabel
               OpStore %4 %23
               OpReturn
               OpFunctionEnd
    )";

    // Push constant are in the vertex Entrypoint
    VkPushConstantRange push_constant_range = {VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16};
    auto pipeline_layout_info = LvlInitStruct<VkPipelineLayoutCreateInfo>();
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    vk_testing::PipelineLayout pipeline_layout(*m_device, pipeline_layout_info);

    VkShaderObj const vs(this, source, VK_SHADER_STAGE_VERTEX_BIT, SPV_ENV_VULKAN_1_0, SPV_SOURCE_ASM);
    VkShaderObj const fs(this, source, VK_SHADER_STAGE_FRAGMENT_BIT, SPV_ENV_VULKAN_1_0, SPV_SOURCE_ASM);

    CreatePipelineHelper pipe(*this);
    pipe.InitInfo();
    pipe.shader_stages_ = {vs.GetStageCreateInfo(), fs.GetStageCreateInfo()};
    pipe.pipeline_layout_ci_ = pipeline_layout_info;
    pipe.InitState();

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkGraphicsPipelineCreateInfo-layout-07987");
    pipe.CreateGraphicsPipeline();
    m_errorMonitor->VerifyFound();

    // Make sure Vertex is ok when used
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pipe.InitState();
    pipe.CreateGraphicsPipeline();
}
