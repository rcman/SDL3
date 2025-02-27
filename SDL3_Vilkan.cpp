#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// Vertex structure
typedef struct {
    float pos[3];
    float color[3];
} Vertex;

// Simple room vertices (cube)
static const Vertex vertices[] = {
    // Floor
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.5f, 0.0f}},
    {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.5f, 0.0f}},
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.5f, 0.0f}},
    {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.5f, 0.0f}},
    // Ceiling
    {{-1.0f,  1.0f, -1.0f}, {0.5f, 0.5f, 0.5f}},
    {{ 1.0f,  1.0f, -1.0f}, {0.5f, 0.5f, 0.5f}},
    {{ 1.0f,  1.0f,  1.0f}, {0.5f, 0.5f, 0.5f}},
    {{-1.0f,  1.0f,  1.0f}, {0.5f, 0.5f, 0.5f}},
    // Walls (simplified)
    {{-1.0f, -1.0f, -1.0f}, {0.7f, 0.7f, 0.7f}},
    {{-1.0f,  1.0f, -1.0f}, {0.7f, 0.7f, 0.7f}},
    {{ 1.0f,  1.0f, -1.0f}, {0.7f, 0.7f, 0.7f}},
    {{ 1.0f, -1.0f, -1.0f}, {0.7f, 0.7f, 0.7f}},
};

static const uint32_t indices[] = {
    0, 1, 2, 2, 3, 0,    // Floor
    4, 5, 6, 6, 7, 4,    // Ceiling
    8, 9, 10, 10, 11, 8  // Back wall
};

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkImage* swapchainImages;
    uint32_t swapchainImageCount;
    VkImageView* swapchainImageViews;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkFramebuffer* framebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
} VulkanContext;

VulkanContext vkContext = {0};
SDL_Window* window;

// Forward declarations
static bool initVulkan(SDL_Window* window);
static void cleanupVulkan(void);
static void createRenderPass(void);
static void createGraphicsPipeline(void);
static void createFramebuffers(void);
static void createCommandBuffers(void);
static void createSyncObjects(void);
static void createVertexBuffer(void);
static void createIndexBuffer(void);

int SDL_AppInit(void** appstate, int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Vulkan 3D Room",
                            SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED,
                            WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        return 1;
    }

    if (!initVulkan(window)) {
        SDL_Log("Vulkan initialization failed");
        return 1;
    }

    return 0;
}

int SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) {
        return 1;
    }
    return 0;
}

int SDL_AppIterate(void* appstate) {
    vkWaitForFences(vkContext.device, 1, &vkContext.inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkContext.device, 1, &vkContext.inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(vkContext.device, vkContext.swapchain, UINT64_MAX,
                         vkContext.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(vkContext.commandBuffer, 0);
    
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(vkContext.commandBuffer, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = vkContext.renderPass;
    renderPassInfo.framebuffer = vkContext.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderPassInfo.renderArea.extent = (VkExtent2D){WINDOW_WIDTH, WINDOW_HEIGHT};
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(vkContext.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext.graphicsPipeline);
    VkBuffer vertexBuffers[] = {vkContext.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(vkContext.commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(vkContext.commandBuffer, vkContext.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(vkContext.commandBuffer, sizeof(indices)/sizeof(indices[0]), 1, 0, 0, 0);
    vkCmdEndRenderPass(vkContext.commandBuffer);
    vkEndCommandBuffer(vkContext.commandBuffer);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkSemaphore waitSemaphores[] = {vkContext.imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkContext.commandBuffer;
    VkSemaphore signalSemaphores[] = {vkContext.renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(vkContext.graphicsQueue, 1, &submitInfo, vkContext.inFlightFence);

    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapchains[] = {vkContext.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    vkQueuePresentKHR(vkContext.graphicsQueue, &presentInfo);

    return 0;
}

void SDL_AppQuit(void* appstate) {
    cleanupVulkan();
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// Vulkan initialization functions (simplified)
static bool initVulkan(SDL_Window* window) {
    // Create instance
    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Vulkan Room";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_win32_surface"};
    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&createInfo, NULL, &vkContext.instance) != VK_SUCCESS) {
        return false;
    }

    // Create surface
    if (!SDL_Vulkan_CreateSurface(window, vkContext.instance, NULL, &vkContext.surface)) {
        return false;
    }

    // Pick physical device and create logical device (simplified)
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, NULL);
    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, devices);
    vkContext.physicalDevice = devices[0];
    free(devices);

    VkDeviceQueueCreateInfo queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    float queuePriority = 1.0f;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    vkCreateDevice(vkContext.physicalDevice, &deviceCreateInfo, NULL, &vkContext.device);
    vkGetDeviceQueue(vkContext.device, 0, 0, &vkContext.graphicsQueue);

    // Create swapchain (simplified)
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainCreateInfo.surface = vkContext.surface;
    swapchainCreateInfo.minImageCount = 2;
    swapchainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainCreateInfo.imageExtent = (VkExtent2D){WINDOW_WIDTH, WINDOW_HEIGHT};
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    vkCreateSwapchainKHR(vkContext.device, &swapchainCreateInfo, NULL, &vkContext.swapchain);

    vkGetSwapchainImagesKHR(vkContext.device, vkContext.swapchain, &vkContext.swapchainImageCount, NULL);
    vkContext.swapchainImages = malloc(sizeof(VkImage) * vkContext.swapchainImageCount);
    vkGetSwapchainImagesKHR(vkContext.device, vkContext.swapchain, &vkContext.swapchainImageCount, vkContext.swapchainImages);

    vkContext.swapchainImageViews = malloc(sizeof(VkImageView) * vkContext.swapchainImageCount);
    for (uint32_t i = 0; i < vkContext.swapchainImageCount; i++) {
        VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = vkContext.swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(vkContext.device, &viewInfo, NULL, &vkContext.swapchainImageViews[i]);
    }

    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();
    createSyncObjects();
    createVertexBuffer();
    createIndexBuffer();

    return true;
}

static void createRenderPass(void) {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    vkCreateRenderPass(vkContext.device, &renderPassInfo, NULL, &vkContext.renderPass);
}

static void createGraphicsPipeline(void) {
    // Simple vertex shader
    const char* vertShaderCode = 
        "#version 450\n"
        "layout(location = 0) in vec3 inPosition;\n"
        "layout(location = 1) in vec3 inColor;\n"
        "layout(location = 0) out vec3 fragColor;\n"
        "void main() {\n"
        "    gl_Position = vec4(inPosition, 1.0);\n"
        "    fragColor = inColor;\n"
        "}\n";

    // Simple fragment shader
    const char* fragShaderCode = 
        "#version 450\n"
        "layout(location = 0) in vec3 fragColor;\n"
        "layout(location = 0) out vec4 outColor;\n"
        "void main() {\n"
        "    outColor = vec4(fragColor, 1.0);\n"
        "}\n";

    // Note: In a real application, you'd want to compile these shaders to SPIR-V
    // This is a simplified example assuming you have a way to convert GLSL to SPIR-V
}

static void createFramebuffers(void) {
    vkContext.framebuffers = malloc(sizeof(VkFramebuffer) * vkContext.swapchainImageCount);
    for (uint32_t i = 0; i < vkContext.swapchainImageCount; i++) {
        VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = vkContext.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &vkContext.swapchainImageViews[i];
        framebufferInfo.width = WINDOW_WIDTH;
        framebufferInfo.height = WINDOW_HEIGHT;
        framebufferInfo.layers = 1;
        vkCreateFramebuffer(vkContext.device, &framebufferInfo, NULL, &vkContext.framebuffers[i]);
    }
}

static void createCommandBuffers(void) {
    VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(vkContext.device, &poolInfo, NULL, &vkContext.commandPool);

    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = vkContext.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(vkContext.device, &allocInfo, &vkContext.commandBuffer);
}

static void createSyncObjects(void) {
    VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(vkContext.device, &semaphoreInfo, NULL, &vkContext.imageAvailableSemaphore);
    vkCreateSemaphore(vkContext.device, &semaphoreInfo, NULL, &vkContext.renderFinishedSemaphore);

    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(vkContext.device, &fenceInfo, NULL, &vkContext.inFlightFence);
}

static void createVertexBuffer(void) {
    // Simplified - in real code, you'd need to create staging buffer and copy to device-local memory
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(vkContext.device, &bufferInfo, NULL, &vkContext.vertexBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkContext.device, vkContext.vertexBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = 0; // Should find suitable memory type in real code
    vkAllocateMemory(vkContext.device, &allocInfo, NULL, &vkContext.vertexBufferMemory);

    vkBindBufferMemory(vkContext.device, vkContext.vertexBuffer, vkContext.vertexBufferMemory, 0);

    void* data;
    vkMapMemory(vkContext.device, vkContext.vertexBufferMemory, 0, sizeof(vertices), 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(vkContext.device, vkContext.vertexBufferMemory);
}

static void createIndexBuffer(void) {
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeof(indices);
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(vkContext.device, &bufferInfo, NULL, &vkContext.indexBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkContext.device, vkContext.indexBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = 0; // Should find suitable memory type in real code
    vkAllocateMemory(vkContext.device, &allocInfo, NULL, &vkContext.indexBufferMemory);

    vkBindBufferMemory(vkContext.device, vkContext.indexBuffer, vkContext.indexBufferMemory, 0);

    void* data;
    vkMapMemory(vkContext.device, vkContext.indexBufferMemory, 0, sizeof(indices), 0, &data);
    memcpy(data, indices, sizeof(indices));
    vkUnmapMemory(vkContext.device, vkContext.indexBufferMemory);
}

static void cleanupVulkan(void) {
    vkDestroyBuffer(vkContext.device, vkContext.indexBuffer, NULL);
    vkFreeMemory(vkContext.device, vkContext.indexBufferMemory, NULL);
    vkDestroyBuffer(vkContext.device, vkContext.vertexBuffer, NULL);
    vkFreeMemory(vkContext.device, vkContext.vertexBufferMemory, NULL);
    vkDestroyFence(vkContext.device, vkContext.inFlightFence, NULL);
    vkDestroySemaphore(vkContext.device, vkContext.renderFinishedSemaphore, NULL);
    vkDestroySemaphore(vkContext.device, vkContext.imageAvailableSemaphore, NULL);
    vkDestroyCommandPool(vkContext.device, vkContext.commandPool, NULL);
    for (uint32_t i = 0; i < vkContext.swapchainImageCount; i++) {
        vkDestroyFramebuffer(vkContext.device, vkContext.framebuffers[i], NULL);
        vkDestroyImageView(vkContext.device, vkContext.swapchainImageViews[i], NULL);
    }
    free(vkContext.framebuffers);
    free(vkContext.swapchainImageViews);
    free(vkContext.swapchainImages);
    vkDestroyPipeline(vkContext.device, vkContext.graphicsPipeline, NULL);
    vkDestroyPipelineLayout(vkContext.device, vkContext.pipelineLayout, NULL);
    vkDestroyRenderPass(vkContext.device, vkContext.renderPass, NULL);
    vkDestroySwapchainKHR(vkContext.device, vkContext.swapchain, NULL);
    vkDestroySurfaceKHR(vkContext.instance, vkContext.surface, NULL);
    vkDestroyDevice(vkContext.device, NULL);
    vkDestroyInstance(vkContext.instance, NULL);
}
