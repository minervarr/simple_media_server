// ============================================
// VULKAN RENDERING - PART 3
// Pipeline, Framebuffers, Commands, Main Loop
// ============================================

// ============================================
// GRAPHICS PIPELINE CREATION
// ============================================
bool create_graphics_pipeline() {
    LOG_INFO("Creating graphics pipeline...");

    // Create shader modules
    VkShaderModule vert_shader_module = create_shader_module(vert_shader_code, sizeof(vert_shader_code));
    VkShaderModule frag_shader_module = create_shader_module(frag_shader_code, sizeof(frag_shader_code));

    if (vert_shader_module == VK_NULL_HANDLE || frag_shader_module == VK_NULL_HANDLE) {
        return false;
    }

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_shader_module,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_shader_module,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    // Vertex input (none - hardcoded triangle)
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0,
    };

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // Viewport and scissor
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)vk_state.swapchain_extent.width,
        .height = (float)vk_state.swapchain_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = vk_state.swapchain_extent,
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
    };

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    // Color blending
    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0,
    };

    VK_CHECK(vkCreatePipelineLayout(vk_state.device, &pipeline_layout_info, NULL, &vk_state.pipeline_layout));

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &color_blending,
        .layout = vk_state.pipeline_layout,
        .renderPass = vk_state.render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
    };

    VK_CHECK(vkCreateGraphicsPipelines(vk_state.device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &vk_state.graphics_pipeline));

    // Cleanup shader modules
    vkDestroyShaderModule(vk_state.device, frag_shader_module, NULL);
    vkDestroyShaderModule(vk_state.device, vert_shader_module, NULL);

    LOG_SUCCESS("Graphics pipeline created");
    return true;
}

// ============================================
// FRAMEBUFFERS CREATION
// ============================================
bool create_framebuffers() {
    LOG_INFO("Creating framebuffers...");

    vk_state.framebuffers = malloc(sizeof(VkFramebuffer) * vk_state.swapchain_image_count);

    for (uint32_t i = 0; i < vk_state.swapchain_image_count; i++) {
        VkImageView attachments[] = {
            vk_state.swapchain_image_views[i]
        };

        VkFramebufferCreateInfo framebuffer_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = vk_state.render_pass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = vk_state.swapchain_extent.width,
            .height = vk_state.swapchain_extent.height,
            .layers = 1,
        };

        VK_CHECK(vkCreateFramebuffer(vk_state.device, &framebuffer_info, NULL, &vk_state.framebuffers[i]));
    }

    LOG_SUCCESS("Created %d framebuffers", vk_state.swapchain_image_count);
    return true;
}

// ============================================
// COMMAND POOL AND BUFFERS
// ============================================
bool create_command_pool() {
    LOG_INFO("Creating command pool...");

    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vk_state.graphics_family_index,
    };

    VK_CHECK(vkCreateCommandPool(vk_state.device, &pool_info, NULL, &vk_state.command_pool));
    LOG_SUCCESS("Command pool created");
    return true;
}

bool create_command_buffers() {
    LOG_INFO("Creating command buffers...");

    vk_state.command_buffers = malloc(sizeof(VkCommandBuffer) * vk_state.swapchain_image_count);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk_state.command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = vk_state.swapchain_image_count,
    };

    VK_CHECK(vkAllocateCommandBuffers(vk_state.device, &alloc_info, vk_state.command_buffers));
    LOG_SUCCESS("Created %d command buffers", vk_state.swapchain_image_count);
    return true;
}

// ============================================
// SYNCHRONIZATION OBJECTS
// ============================================
bool create_sync_objects() {
    LOG_INFO("Creating synchronization objects...");

    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    VK_CHECK(vkCreateSemaphore(vk_state.device, &semaphore_info, NULL, &vk_state.image_available_semaphore));
    VK_CHECK(vkCreateSemaphore(vk_state.device, &semaphore_info, NULL, &vk_state.render_finished_semaphore));
    VK_CHECK(vkCreateFence(vk_state.device, &fence_info, NULL, &vk_state.in_flight_fence));

    LOG_SUCCESS("Synchronization objects created");
    return true;
}

// ============================================
// RECORD COMMAND BUFFER
// ============================================
void record_command_buffer(VkCommandBuffer cmd_buffer, uint32_t image_index) {
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    vkBeginCommandBuffer(cmd_buffer, &begin_info);

    VkClearValue clear_color = {{{0.1f, 0.1f, 0.1f, 1.0f}}};

    VkRenderPassBeginInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vk_state.render_pass,
        .framebuffer = vk_state.framebuffers[image_index],
        .renderArea.offset = {0, 0},
        .renderArea.extent = vk_state.swapchain_extent,
        .clearValueCount = 1,
        .pClearValues = &clear_color,
    };

    vkCmdBeginRenderPass(cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_state.graphics_pipeline);
    vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmd_buffer);

    vkEndCommandBuffer(cmd_buffer);
}

// ============================================
// CLEANUP FUNCTIONS
// ============================================
void cleanup_swapchain() {
    for (uint32_t i = 0; i < vk_state.swapchain_image_count; i++) {
        vkDestroyFramebuffer(vk_state.device, vk_state.framebuffers[i], NULL);
        vkDestroyImageView(vk_state.device, vk_state.swapchain_image_views[i], NULL);
    }

    free(vk_state.framebuffers);
    free(vk_state.swapchain_image_views);
    free(vk_state.swapchain_images);

    vkDestroySwapchainKHR(vk_state.device, vk_state.swapchain, NULL);
}

void cleanup_vulkan() {
    LOG_INFO("Cleaning up Vulkan...");

    vkDeviceWaitIdle(vk_state.device);

    vkDestroySemaphore(vk_state.device, vk_state.render_finished_semaphore, NULL);
    vkDestroySemaphore(vk_state.device, vk_state.image_available_semaphore, NULL);
    vkDestroyFence(vk_state.device, vk_state.in_flight_fence, NULL);

    vkDestroyCommandPool(vk_state.device, vk_state.command_pool, NULL);
    free(vk_state.command_buffers);

    cleanup_swapchain();

    vkDestroyPipeline(vk_state.device, vk_state.graphics_pipeline, NULL);
    vkDestroyPipelineLayout(vk_state.device, vk_state.pipeline_layout, NULL);
    vkDestroyRenderPass(vk_state.device, vk_state.render_pass, NULL);

    vkDestroyDevice(vk_state.device, NULL);
    vkDestroySurfaceKHR(vk_state.instance, vk_state.surface, NULL);

#ifdef DEBUG_VERBOSE
    if (vk_state.debug_messenger) {
        vkDestroyDebugUtilsMessengerEXT(vk_state.instance, vk_state.debug_messenger, NULL);
    }
#endif

    vkDestroyInstance(vk_state.instance, NULL);
    unload_vulkan_loader();

    LOG_SUCCESS("Vulkan cleaned up");
}

// ============================================
// SOKOL_APP CALLBACKS
// ============================================
static void init(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  VULKAN MEDIA PLAYER - NATIVE IMPLEMENTATION\n");
    printf("  Maximum versatility for future hardware\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");

    LOG_INFO("Platform: %s",
#if defined(_WIN32)
        "Windows"
#elif defined(__ANDROID__)
        "Android"
#elif defined(__linux__)
        "Linux"
#else
        "Unknown"
#endif
    );

    // Initialize sokol_time
    stm_setup();
    vk_state.last_time = stm_now();

    // Load Vulkan loader
    if (!load_vulkan_loader()) {
        LOG_ERROR("Failed to load Vulkan loader");
        vk_state.error_count++;
        return;
    }
    LOG_SUCCESS("Vulkan loader loaded");

    // Load global functions
    if (!load_global_functions()) {
        vk_state.error_count++;
        return;
    }

    // Create instance
    if (!create_instance()) {
        vk_state.error_count++;
        return;
    }

    // Load instance functions
    if (!load_instance_functions()) {
        vk_state.error_count++;
        return;
    }

    // Create surface
    if (!create_surface()) {
        vk_state.error_count++;
        return;
    }

    // Pick physical device
    if (!pick_physical_device()) {
        vk_state.error_count++;
        return;
    }

    // Create logical device
    if (!create_logical_device()) {
        vk_state.error_count++;
        return;
    }

    // Load device functions
    if (!load_device_functions()) {
        vk_state.error_count++;
        return;
    }

    // Get device queues
    if (!get_device_queues()) {
        vk_state.error_count++;
        return;
    }

    // Create swapchain
    if (!create_swapchain()) {
        vk_state.error_count++;
        return;
    }

    // Create image views
    if (!create_image_views()) {
        vk_state.error_count++;
        return;
    }

    // Create render pass
    if (!create_render_pass()) {
        vk_state.error_count++;
        return;
    }

    // Create graphics pipeline
    if (!create_graphics_pipeline()) {
        vk_state.error_count++;
        return;
    }

    // Create framebuffers
    if (!create_framebuffers()) {
        vk_state.error_count++;
        return;
    }

    // Create command pool
    if (!create_command_pool()) {
        vk_state.error_count++;
        return;
    }

    // Create command buffers
    if (!create_command_buffers()) {
        vk_state.error_count++;
        return;
    }

    // Create sync objects
    if (!create_sync_objects()) {
        vk_state.error_count++;
        return;
    }

    vk_state.initialized = true;

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ✓ VULKAN INITIALIZATION COMPLETE\n");
    printf("  Errors: %d\n", vk_state.error_count);
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");

    LOG_INFO("Will auto-quit after %d frames for testing", TEST_DURATION_FRAMES);
}

static void frame(void) {
    if (!vk_state.initialized) {
        return;
    }

    vk_state.frame_count++;

    // Calculate delta time
    uint64_t now = stm_now();
    double dt = stm_sec(stm_diff(now, vk_state.last_time));
    vk_state.last_time = now;

    // Periodic logging
    if (vk_state.frame_count % DEBUG_FRAME_LOG == 0) {
        LOG_DEBUG("Frame %d | dt=%.3fms | fps=~%.1f",
                  vk_state.frame_count, dt * 1000.0, 1.0 / dt);
    }

    // Wait for previous frame
    vkWaitForFences(vk_state.device, 1, &vk_state.in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk_state.device, 1, &vk_state.in_flight_fence);

    // Acquire next image
    uint32_t image_index;
    vkAcquireNextImageKHR(vk_state.device, vk_state.swapchain, UINT64_MAX,
                         vk_state.image_available_semaphore, VK_NULL_HANDLE, &image_index);

    // Record command buffer
    vkResetCommandBuffer(vk_state.command_buffers[image_index], 0);
    record_command_buffer(vk_state.command_buffers[image_index], image_index);

    // Submit
    VkSemaphore wait_semaphores[] = {vk_state.image_available_semaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signal_semaphores[] = {vk_state.render_finished_semaphore};

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &vk_state.command_buffers[image_index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores,
    };

    vkQueueSubmit(vk_state.graphics_queue, 1, &submit_info, vk_state.in_flight_fence);

    // Present
    VkSwapchainKHR swapchains[] = {vk_state.swapchain};
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &image_index,
    };

    vkQueuePresentKHR(vk_state.present_queue, &present_info);

    // Auto-quit for testing
    if (vk_state.frame_count >= TEST_DURATION_FRAMES) {
        LOG_INFO("Reached %d frames, auto-quitting for test", TEST_DURATION_FRAMES);
        LOG_INFO("Test completed successfully!");
        sapp_request_quit();
    }
}

static void cleanup(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  CLEANUP & STATISTICS\n");
    printf("═══════════════════════════════════════════════════════════\n");

    LOG_INFO("Total frames rendered: %d", vk_state.frame_count);
    LOG_INFO("Total errors: %d", vk_state.error_count);

    cleanup_vulkan();

    printf("═══════════════════════════════════════════════════════════\n");

    if (vk_state.error_count > 0) {
        printf("  ⚠ COMPLETED WITH %d ERRORS\n", vk_state.error_count);
    } else {
        printf("  ✓ COMPLETED SUCCESSFULLY\n");
    }

    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
}

static void input_event(const sapp_event* e) {
    switch (e->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            if (e->key_code == SAPP_KEYCODE_ESCAPE) {
                LOG_INFO("ESC pressed - requesting quit");
                sapp_request_quit();
            }
            break;
        default:
            break;
    }
}
