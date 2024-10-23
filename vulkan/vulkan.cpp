#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

// Function to check Vulkan errors
void checkVulkanResult(VkResult result, const char* msg) {
    if (result != VK_SUCCESS) {
        std::cerr << msg << ": Error Code " << result << std::endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    // Initialize SDL with Vulkan support
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create SDL window with Vulkan flag
    SDL_Window* window = SDL_CreateWindow("SDL3 Vulkan Window", 640, 480, SDL_WINDOW_VULKAN);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create Vulkan instance
    VkInstance instance;
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "SDL Vulkan App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get Vulkan extensions required by SDL
    unsigned int extensionCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr)) {
        std::cerr << "SDL_Vulkan_GetInstanceExtensions failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    std::vector<const char*> extensions(extensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data())) {
        std::cerr << "SDL_Vulkan_GetInstanceExtensions failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Create Vulkan instance
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    checkVulkanResult(result, "Failed to create Vulkan instance");

    // Create a Vulkan surface for the SDL window
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
        std::cerr << "SDL_Vulkan_CreateSurface failed: " << SDL_GetError() << std::endl;
        vkDestroyInstance(instance, nullptr);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Main loop
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Render loop or additional Vulkan logic would go here
        SDL_Delay(16);  // Simulate rendering frame delay
    }

    // Clean up Vulkan and SDL resources
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
