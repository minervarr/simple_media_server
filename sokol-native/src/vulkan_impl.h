// ============================================
// VULKAN IMPLEMENTATION - PART 2
// Swapchain, Pipeline, Rendering
// ============================================

// ============================================
// SWAPCHAIN CREATION
// ============================================
bool create_swapchain() {
    LOG_INFO("Creating swapchain...");

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_state.physical_device, vk_state.surface, &capabilities);

    // Get surface formats
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk_state.physical_device, vk_state.surface, &format_count, NULL);
    VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk_state.physical_device, vk_state.surface, &format_count, formats);

    // Choose format (prefer SRGB)
    VkSurfaceFormatKHR surface_format = formats[0];
    for (uint32_t i = 0; i < format_count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = formats[i];
            break;
        }
    }
    free(formats);

    // Get present modes
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk_state.physical_device, vk_state.surface, &present_mode_count, NULL);
    VkPresentModeKHR* present_modes = malloc(sizeof(VkPresentModeKHR) * present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk_state.physical_device, vk_state.surface, &present_mode_count, present_modes);

    // Choose present mode (prefer MAILBOX, fallback to FIFO)
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < present_mode_count; i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = present_modes[i];
            break;
        }
    }
    free(present_modes);

    // Choose swap extent
    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        extent.width = sapp_width();
        extent.height = sapp_height();

        if (extent.width < capabilities.minImageExtent.width) extent.width = capabilities.minImageExtent.width;
        if (extent.width > capabilities.maxImageExtent.width) extent.width = capabilities.maxImageExtent.width;
        if (extent.height < capabilities.minImageExtent.height) extent.height = capabilities.minImageExtent.height;
        if (extent.height > capabilities.maxImageExtent.height) extent.height = capabilities.maxImageExtent.height;
    }

    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk_state.surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    uint32_t queue_family_indices[] = {vk_state.graphics_family_index, vk_state.present_family_index};
    if (vk_state.graphics_family_index != vk_state.present_family_index) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VK_CHECK(vkCreateSwapchainKHR(vk_state.device, &create_info, NULL, &vk_state.swapchain));

    vk_state.swapchain_format = surface_format.format;
    vk_state.swapchain_extent = extent;

    LOG_SUCCESS("Swapchain created (%dx%d, %d images)", extent.width, extent.height, image_count);
    return true;
}

// ============================================
// IMAGE VIEWS CREATION
// ============================================
bool create_image_views() {
    LOG_INFO("Creating image views...");

    vkGetSwapchainImagesKHR(vk_state.device, vk_state.swapchain, &vk_state.swapchain_image_count, NULL);
    vk_state.swapchain_images = malloc(sizeof(VkImage) * vk_state.swapchain_image_count);
    vkGetSwapchainImagesKHR(vk_state.device, vk_state.swapchain, &vk_state.swapchain_image_count, vk_state.swapchain_images);

    vk_state.swapchain_image_views = malloc(sizeof(VkImageView) * vk_state.swapchain_image_count);

    for (uint32_t i = 0; i < vk_state.swapchain_image_count; i++) {
        VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vk_state.swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk_state.swapchain_format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };

        VK_CHECK(vkCreateImageView(vk_state.device, &create_info, NULL, &vk_state.swapchain_image_views[i]));
    }

    LOG_SUCCESS("Created %d image views", vk_state.swapchain_image_count);
    return true;
}

// ============================================
// RENDER PASS CREATION
// ============================================
bool create_render_pass() {
    LOG_INFO("Creating render pass...");

    VkAttachmentDescription color_attachment = {
        .format = vk_state.swapchain_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VK_CHECK(vkCreateRenderPass(vk_state.device, &render_pass_info, NULL, &vk_state.render_pass));
    LOG_SUCCESS("Render pass created");
    return true;
}

// ============================================
// SHADER MODULE CREATION
// ============================================
VkShaderModule create_shader_module(const uint32_t* code, size_t code_size) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code_size,
        .pCode = code,
    };

    VkShaderModule shader_module;
    if (vkCreateShaderModule(vk_state.device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
        LOG_ERROR("Failed to create shader module");
        return VK_NULL_HANDLE;
    }

    return shader_module;
}

// ============================================
// SPIR-V SHADERS (Hardcoded for simplicity)
// ============================================
// Simple vertex shader: fullscreen triangle
static const uint32_t vert_shader_code[] = {
    0x07230203,0x00010000,0x000d000a,0x00000020,
    0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,
    0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0008000f,0x00000000,0x00000004,0x6e69616d,
    0x00000000,0x0000000a,0x0000001a,0x0000001f,
    0x00030003,0x00000002,0x000001c2,0x000a0004,
    0x475f4c47,0x4c474f4f,0x70635f45,0x74735f70,
    0x5f656c79,0x656e696c,0x7269645f,0x69746365,
    0x00006576,0x00080004,0x475f4c47,0x4c474f4f,
    0x6e695f45,0x64756c63,0x69645f65,0x74636572,
    0x00657669,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00050005,0x0000000a,0x6f506c67,
    0x69746973,0x00006e6f,0x00060005,0x0000000e,
    0x505f6c67,0x65567265,0x78657472,0x00000000,
    0x00060006,0x0000000e,0x00000000,0x505f6c67,
    0x7469736f,0x006e6f69,0x00070006,0x0000000e,
    0x00000001,0x505f6c67,0x746e696f,0x657a6953,
    0x00000000,0x00070006,0x0000000e,0x00000002,
    0x435f6c67,0x4470696c,0x61747369,0x0065636e,
    0x00070006,0x0000000e,0x00000003,0x435f6c67,
    0x446c6c75,0x61747369,0x0065636e,0x00030005,
    0x00000010,0x00000000,0x00060005,0x0000001a,
    0x67617266,0x6f6c6f43,0x00000072,0x00050005,
    0x0000001f,0x65566e69,0x78657472,0x00444900,
    0x00050048,0x0000000e,0x00000000,0x0000000b,
    0x00000000,0x00050048,0x0000000e,0x00000001,
    0x0000000b,0x00000001,0x00050048,0x0000000e,
    0x00000002,0x0000000b,0x00000003,0x00050048,
    0x0000000e,0x00000003,0x0000000b,0x00000004,
    0x00030047,0x0000000e,0x00000002,0x00040047,
    0x0000001a,0x0000001e,0x00000000,0x00040047,
    0x0000001f,0x0000001e,0x00000000,0x00020013,
    0x00000002,0x00030021,0x00000003,0x00000002,
    0x00030016,0x00000006,0x00000020,0x00040017,
    0x00000007,0x00000006,0x00000004,0x00040020,
    0x00000008,0x00000003,0x00000007,0x0004003b,
    0x00000008,0x0000000a,0x00000003,0x00040015,
    0x0000000b,0x00000020,0x00000000,0x0004002b,
    0x0000000b,0x0000000c,0x00000001,0x0004001c,
    0x0000000d,0x00000006,0x0000000c,0x0006001e,
    0x0000000e,0x00000007,0x00000006,0x0000000d,
    0x0000000d,0x00040020,0x0000000f,0x00000003,
    0x0000000e,0x0004003b,0x0000000f,0x00000010,
    0x00000003,0x00040015,0x00000011,0x00000020,
    0x00000001,0x0004002b,0x00000011,0x00000012,
    0x00000000,0x00040020,0x00000018,0x00000003,
    0x00000007,0x0004003b,0x00000018,0x0000001a,
    0x00000003,0x00040020,0x0000001d,0x00000001,
    0x00000011,0x0004003b,0x0000001d,0x0000001f,
    0x00000001,0x0004002b,0x00000011,0x00000022,
    0x00000001,0x0004002b,0x00000006,0x00000024,
    0x3f800000,0x00050036,0x00000002,0x00000004,
    0x00000000,0x00000003,0x000200f8,0x00000005,
    0x0004003d,0x00000011,0x00000020,0x0000001f,
    0x000500aa,0x00000012,0x00000021,0x00000020,
    0x00000012,0x000300f7,0x00000027,0x00000000,
    0x000400fa,0x00000021,0x00000025,0x00000026,
    0x000200f8,0x00000025,0x00050051,0x00000006,
    0x00000028,0x0000001a,0x00000000,0x00050051,
    0x00000006,0x00000029,0x0000001a,0x00000001,
    0x00060050,0x00000007,0x0000002a,0x00000028,
    0x00000029,0x00000024,0x0003003e,0x0000000a,
    0x0000002a,0x000200f9,0x00000027,0x000200f8,
    0x00000026,0x000200f9,0x00000027,0x000200f8,
    0x00000027,0x000100fd,0x00010038
};

// Simple fragment shader: solid color
static const uint32_t frag_shader_code[] = {
    0x07230203,0x00010000,0x000d000a,0x0000000d,
    0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,
    0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0007000f,0x00000004,0x00000004,0x6e69616d,
    0x00000000,0x00000009,0x0000000b,0x00030010,
    0x00000004,0x00000007,0x00030003,0x00000002,
    0x000001c2,0x000a0004,0x475f4c47,0x4c474f4f,
    0x70635f45,0x74735f70,0x5f656c79,0x656e696c,
    0x7269645f,0x69746365,0x00006576,0x00080004,
    0x475f4c47,0x4c474f4f,0x6e695f45,0x64756c63,
    0x69645f65,0x74636572,0x00657669,0x00040005,
    0x00000004,0x6e69616d,0x00000000,0x00050005,
    0x00000009,0x4374756f,0x726f6c6f,0x00000000,
    0x00050005,0x0000000b,0x67617266,0x6f6c6f43,
    0x00000072,0x00040047,0x00000009,0x0000001e,
    0x00000000,0x00040047,0x0000000b,0x0000001e,
    0x00000000,0x00020013,0x00000002,0x00030021,
    0x00000003,0x00000002,0x00030016,0x00000006,
    0x00000020,0x00040017,0x00000007,0x00000006,
    0x00000004,0x00040020,0x00000008,0x00000003,
    0x00000007,0x0004003b,0x00000008,0x00000009,
    0x00000003,0x00040020,0x0000000a,0x00000001,
    0x00000007,0x0004003b,0x0000000a,0x0000000b,
    0x00000001,0x00050036,0x00000002,0x00000004,
    0x00000000,0x00000003,0x000200f8,0x00000005,
    0x0004003d,0x00000007,0x0000000c,0x0000000b,
    0x0003003e,0x00000009,0x0000000c,0x000100fd,
    0x00010038
};

// Continue in next message...
