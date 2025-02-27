#include <SDL3/SDL.h>
#include <SDL3/SDL_ttf.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MENU_ITEMS 4
#define FONT_SIZE 32

typedef struct {
    const char *text;
    SDL_Rect rect;
    bool hovered;
} MenuItem;

int main(int argc, char *argv[]) {
    // Initialize SDL and TTF
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        SDL_Log("TTF initialization failed: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // Create window and renderer
    SDL_Window *window = SDL_CreateWindow("Menu App",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load font
    TTF_Font *font = TTF_OpenFont("arial.ttf", FONT_SIZE);
    if (!font) {
        SDL_Log("Font loading failed: %s", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Menu items
    const char *menu_texts[MENU_ITEMS] = {"Start", "Options", "Credits", "Quit"};
    MenuItem menu_items[MENU_ITEMS];
    
    // Initialize menu items
    int y_offset = WINDOW_HEIGHT / 4;
    for (int i = 0; i < MENU_ITEMS; i++) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, menu_texts[i], (SDL_Color){255, 255, 255, 255});
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        
        menu_items[i].text = menu_texts[i];
        menu_items[i].rect.x = (WINDOW_WIDTH - surface->w) / 2;
        menu_items[i].rect.y = y_offset;
        menu_items[i].rect.w = surface->w;
        menu_items[i].rect.h = surface->h;
        menu_items[i].hovered = false;
        
        y_offset += surface->h + 20;
        
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    for (int i = 0; i < MENU_ITEMS; i++) {
                        menu_items[i].hovered = SDL_PointInRect(
                            &(SDL_Point){event.motion.x, event.motion.y},
                            &menu_items[i].rect);
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        for (int i = 0; i < MENU_ITEMS; i++) {
                            if (menu_items[i].hovered) {
                                SDL_Log("Selected: %s", menu_items[i].text);
                                if (i == 3) { // Quit
                                    running = false;
                                }
                            }
                        }
                    }
                    break;
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int i = 0; i < MENU_ITEMS; i++) {
            SDL_Color color = menu_items[i].hovered ? 
                (SDL_Color){255, 255, 0, 255} :  // Yellow when hovered
                (SDL_Color){255, 255, 255, 255}; // White normally

            SDL_Surface *surface = TTF_RenderText_Solid(font, menu_items[i].text, color);
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            
            SDL_RenderTexture(renderer, texture, NULL, &menu_items[i].rect);
            
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
