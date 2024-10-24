#include <SDL3/SDL.h>
#include <iostream>

// Menu options
const char* menuItems[] = {"Start Game", "Options", "Exit"};
const int menuItemsCount = sizeof(menuItems) / sizeof(menuItems[0]);

int main(int argc, char* argv[]) {
    // Initialize SDL3
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create an SDL Window
    SDL_Window* window = SDL_CreateWindow("SDL3 Menu", 640, 480, SDL_WINDOW_OPENGL);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    int selectedItem = 0;  // Current selected menu item
    SDL_Event event;

    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        selectedItem = (selectedItem - 1 + menuItemsCount) % menuItemsCount; // Navigate up
                        break;
                    case SDLK_DOWN:
                        selectedItem = (selectedItem + 1) % menuItemsCount;  // Navigate down
                        break;
                    case SDLK_RETURN:  // Enter key
                        if (selectedItem == menuItemsCount - 1) {
                            running = false;  // Exit if "Exit" is selected
                        } else {
                            std::cout << "Selected: " << menuItems[selectedItem] << std::endl;
                        }
                        break;
                }
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Black background
        SDL_RenderClear(renderer);

        // Simple rendering for text-like menu (replace with proper font rendering if needed)
        for (int i = 0; i < menuItemsCount; ++i) {
            SDL_Color color = (i == selectedItem) ? SDL_Color{255, 0, 0, 255} : SDL_Color{255, 255, 255, 255};  // Red for selected item
            // You would ideally use SDL_ttf to render real text, here we just simulate it with debug output
            std::cout << (i == selectedItem ? ">> " : "   ") << menuItems[i] << std::endl;
        }

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
