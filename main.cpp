#include <iostream>
#include <SDL2/SDL.h>
#include <atomic>
#include <random>
#include <array>
#include <iterator>
#include <chrono>

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

struct Color
{
    uint8_t r, g, b, a = 0;
};

Color mix(Color c1, Color c2)
{
    if(c1.a == 0) c1.a = 1;
    if(c2.a == 0) c2.a = 1;

    int a = (255 - c1.a) * c2.a + c1.a;
    int r = ((255 - c1.a) * c2.a * c2.r + c1.a * c1.r) / a;
    int g = ((255 - c1.a) * c2.a * c2.g + c1.a * c1.g) / a;
    int b = ((255 - c1.a) * c2.a * c2.b + c1.a * c1.b) / a;

    return Color {
        static_cast<uint8_t>(r < 255 ? r : 255),
        static_cast<uint8_t>(g < 255 ? g : 255),
        static_cast<uint8_t>(b < 255 ? b : 255),
        static_cast<uint8_t>(a < 255 ? b : 255),
    };
}

template<int ROWS, int COLUMNS, int MAX_BARS, int MAX_INVERSE_SPEED>
class Simulation
{
public:
    Simulation()
    {
        std::mt19937_64 engine{
            static_cast<unsigned long>(std::chrono::system_clock::now().time_since_epoch().count())
        };

        position_random_engine.seed(engine());
        color_random_engine.seed(engine());
        lifetime_random_engine.seed(engine());
        inv_random_engine.seed(engine());
    }

    void tick()
    {
        int i = 0;
        for(auto& bar: bars)
        {
            if(bar.alive)
            {
                if(bar.ticks_to_wait <= 0)
                {
                    if(bar.current_row != -1)
                    {
                        int a = 400 - 255 * bar.current_row / bar.lifetime;
                        screen[bar.current_row][bar.column] =
                            mix({
                                    bar.color.r,
                                    bar.color.g,
                                    bar.color.b,
                                    static_cast<uint8_t>(a < 255 ? a : 255)
                                },
                                screen[bar.current_row][bar.column]
                            );
                    }
                    bar.current_row++;
                    if(bar.current_row < bar.lifetime)
                        bar.ticks_to_wait = bar.inverse_speed;
                    else bar.alive = false;
                }
                else bar.ticks_to_wait--;
            }
            else
            {
                bar = Bar{
                    true,
                    colors[random_color()],
                    random_lifetime(),
                    random_inv_speed(),
                    random_position(),
                    -1,
                    0
                };
            }
        }
    }

    uint8_t* image_data() { return &screen[0][0].r; }
    uint8_t* image_data_end() { return image_data() + (ROWS - 1) * (COLUMNS - 1) * sizeof(Color) + 1; }

private:
    Color screen[ROWS][COLUMNS]{};
    std::mt19937 position_random_engine;
    std::mt19937 color_random_engine;
    std::mt19937 lifetime_random_engine;
    std::mt19937 inv_random_engine;

    auto random_position()
    {
        return std::uniform_int_distribution<int>{0, COLUMNS - 1}(position_random_engine);
    }

    auto random_color()
    {
        return std::uniform_int_distribution<int>{0, static_cast<int>(std::size(colors)) - 1}(color_random_engine);
    }

    auto random_lifetime()
    {
        return std::uniform_int_distribution<int>{ROWS / 4, ROWS - 1}(lifetime_random_engine);
    }

    auto random_inv_speed()
    {
        return std::uniform_int_distribution<int>{1, MAX_INVERSE_SPEED}(inv_random_engine);
    }

/*
    const Color colors[6] = {
        Color{0xE3, 0x17, 0x0A, 0xFF},
        Color{0xEC, 0xA4, 0x00, 0xFF},
        Color{0x00, 0x78, 0xFF, 0xFF},
        Color{0xF9, 0xDC, 0x5C, 0xFF},
        Color{0x3C, 0x17, 0x42, 0xFF},
        Color{0xE3, 0x17, 0x0A, 0xFF},
    };*/

    const Color colors[5] = {
        Color{0x37, 0x12, 0x3C, 0xFF},
        Color{0x71, 0x67, 0x7C, 0xFF},
        Color{0xA9, 0x9F, 0x96, 0xFF},
        Color{0xDD, 0xA7, 0x7B, 0xFF},
        Color{0x94, 0x5D, 0x5E, 0xFF},
    };

    struct Bar
    {
        bool alive = false;
        Color color;
        int lifetime;
        int inverse_speed;
        int column;

        int current_row;
        int ticks_to_wait;
    };

    Bar bars[MAX_BARS]{};
};

int main()
{
    auto main_loop = [](){
        constexpr int ROWS = 200;
        constexpr int COLUMNS = 200;
        static Simulation<ROWS, COLUMNS, 25, 4> simulation;
        static SDL_Surface* surface = SDL_CreateRGBSurface(
                0,
                COLUMNS,
                ROWS,
                32,
                0x000000FF,
                0x0000FF00,
                0x00FF0000,
                0xFF000000
            );
        static SDL_Renderer* render = SDL_CreateRenderer(
                SDLContext::get().window(),
                -1,
                SDL_RENDERER_SOFTWARE
            );

        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            if(e.type == SDL_QUIT)
                SDLContext::get().set_should_close(true);
        }

        simulation.tick();

        SDL_LockSurface(surface);
        std::copy(
                simulation.image_data(),
                simulation.image_data_end(),
                static_cast<uint8_t*>(surface->pixels)
            );
        SDL_UnlockSurface(surface);

        int w, h;
        SDL_GetWindowSize(SDLContext::get().window(), &w, &h);

        auto texture = SDL_CreateTextureFromSurface(render, surface);
        SDL_RenderCopy(
            render,
            texture,
            nullptr,
            nullptr
            );

        SDL_RenderPresent(render);
        SDL_DestroyTexture(texture);
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
