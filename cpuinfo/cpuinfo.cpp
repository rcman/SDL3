#include <SDL3/SDL.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // Get the number of CPU cores
    int cpu_count = SDL_GetCPUCount();
    printf("CPU Count: %d\n", cpu_count);

    // Get the L1 cache line size
    int cache_size = SDL_GetCPUCacheLineSize();
    printf("CPU Cache Line Size: %d bytes\n", cache_size);

    // Check for specific CPU features
    if (SDL_HasRDTSC()) {
        printf("CPU supports RDTSC\n");
    }
    if (SDL_HasAltiVec()) {
        printf("CPU supports AltiVec\n");
    }
    if (SDL_HasSSE()) {
        printf("CPU supports SSE\n");
    }
    if (SDL_HasSSE2()) {
        printf("CPU supports SSE2\n");
    }
    if (SDL_HasSSE3()) {
        printf("CPU supports SSE3\n");
    }
    if (SDL_HasSSE41()) {
        printf("CPU supports SSE4.1\n");
    }
    if (SDL_HasSSE42()) {
        printf("CPU supports SSE4.2\n");
    }

    // Get the system RAM size
    int ram_size = SDL_GetSystemRAM();
    printf("System RAM: %d MB\n", ram_size);

    // Clean up SDL
    SDL_Quit();
    
    return 0;
}
