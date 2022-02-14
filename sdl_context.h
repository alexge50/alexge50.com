//
// Created by alex on 13/02/2022.
//

#ifndef BACKGROUND_SDL_CONTEXT_H
#define BACKGROUND_SDL_CONTEXT_H

#include <SDL2/SDL.h>
#include <atomic>

class SDLContext
{
private:
    SDLContext():
            should_close_{false} {
        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL initialization error: " << SDL_GetError() << std::endl;
            exit(-1);
        }

        if((
                   window_ = SDL_CreateWindow(
                           "alexge50.com",
                           SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED,
                           512,
                           512,
                           SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
                   )) == nullptr) {
            std::cerr << "creating window failed: " << SDL_GetError() << std::endl;
            exit(-1);
        }
    }

    ~SDLContext() {
#ifndef __EMSCRIPTEN__
        SDL_DestroyWindow(window_);
        SDL_Quit();
#endif
    }

public:
    static SDLContext& get() {
        static SDLContext context{};
        return context;
    }

    auto window() { return window_; }

    void set_should_close(bool x) { should_close_ = x; }
    auto should_close() { return should_close_.load(); }

private:
    SDL_Window* window_;
    std::atomic<bool> should_close_ ;
};

#endif //BACKGROUND_SDL_CONTEXT_H
