#include <iostream>
#include <SDL2/SDL.h>
#include <atomic>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

class SDLContext
{
private:
    SDLContext():
        should_close_{false}
    {
        if(SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cerr << "SDL initialization error: " << SDL_GetError() << std::endl;
            exit(-1);
        }

        if((
               window_ = SDL_CreateWindow(
                   "background",
                   SDL_WINDOWPOS_UNDEFINED,
                   SDL_WINDOWPOS_UNDEFINED,
                   512,
                   512,
                   SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
               )) == nullptr)
        {
            std::cerr << "creating window failed: " << SDL_GetError() << std::endl;
            exit(-1);
        }
    }

    ~SDLContext()
    {
        #ifndef __EMSCRIPTEN__
        SDL_DestroyWindow(window_);
        SDL_Quit();
        #endif
    }

public:
    static SDLContext& get()
    {
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

int main()
{
    auto main_loop = [](){
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            if(e.type == SDL_QUIT)
                SDLContext::get().set_should_close(true);
        }

        int w, h;
        SDL_GetWindowSize(SDLContext::get().window(), &w, &h);
        std::cout << w << "x" << h << std::endl;
    };

    #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(main_loop, 60, 1);
    #else
        while(!SDLContext::get().should_close())
        {
            main_loop();
            SDL_Delay(10);
        }
    #endif

    return 0;
}
