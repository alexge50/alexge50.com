#include <iostream>
#include <SDL2/SDL.h>
#include <atomic>
#include <random>
#include <array>
#include <iterator>
#include <chrono>
#include <optional>

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

class Direction
{
public:
    enum Directions
    {
        NORTH = 0,
        EAST,
        SOUTH,
        WEST
    };

    Direction(Directions d = NORTH): direction{d} {}

    Direction& operator++(int)
    {
        int x = direction + 1;
        if(x > WEST)
            x = NORTH;
        direction = static_cast<Directions>(x);
    }

    Direction& operator--(int)
    {
        int x = direction - 1;
        if(x < NORTH)
            x = WEST;
        direction = static_cast<Directions>(x);
    }

    Directions get() { return direction; }

    int get_direction_vector_row()
    {
        int dir[] = {-1, 0, 1, 0};
        return dir[direction];
    }

    int get_direction_vector_column()
    {
        int dir[] = {0, 1, 0, -1};
        return dir[direction];
    }

private:
    Directions direction;
};

template<int ROWS, int COLUMNS, int MAX_BARS, int MAX_INVERSE_SPEED>
class Simulation
{
public:
    Simulation()
    {
        std::mt19937_64 engine{static_cast<unsigned long>(std::chrono::system_clock::now().time_since_epoch().count())};

        for(auto& ant: ants)
            ant = {
                std::uniform_int_distribution{0, ROWS}(engine),
                std::uniform_int_distribution{0, COLUMNS}(engine),
                colors[std::uniform_int_distribution{0, sizeof(colors)}(engine)],
                static_cast<Direction::Directions>(std::uniform_int_distribution{0, 4}(engine))
        };
    }

    void tick()
    {
        for(auto& ant: ants)
        {
            if(ant.row >= ROWS)
                ant.row = 0;
            else if(ant.row < 0)
                ant.row = ROWS - 1;

            if(ant.column >= COLUMNS)
                ant.column = 0;
            else if(ant.column < 0)
                ant.column = COLUMNS - 1;

            if(colony[ant.row][ant.column])
            {
                colony[ant.row][ant.column] = std::nullopt;
                screen[ant.row][ant.column] = Color{0, 0, 0, 255};
                ant.direction--;
                ant.row += ant.direction.get_direction_vector_row();
                ant.column += ant.direction.get_direction_vector_column();
            }
            else
            {
                colony[ant.row][ant.column] = ant.color;
                screen[ant.row][ant.column] = ant.color;
                ant.direction++;
                ant.row += ant.direction.get_direction_vector_row();
                ant.column += ant.direction.get_direction_vector_column();
            }
        }
    }

    uint8_t* image_data() { return &screen[0][0].r; }
    uint8_t* image_data_end() { return image_data() + (ROWS - 1) * (COLUMNS - 1) * sizeof(Color) + 1; }

private:
    Color screen[ROWS][COLUMNS]{};
    std::optional<Color> colony[ROWS][COLUMNS];
    struct {
        int row;
        int column;
        Color color;
        Direction direction;
    } ants [2000];

    const Color colors[5] = {
        Color{0x37, 0x12, 0x3C, 0xFF},
        Color{0x71, 0x67, 0x7C, 0xFF},
        Color{0xA9, 0x9F, 0x96, 0xFF},
        Color{0xDD, 0xA7, 0x7B, 0xFF},
        Color{0x94, 0x5D, 0x5E, 0xFF},
    };

};

int main()
{
    auto main_loop = [](){
        constexpr int ROWS = 256;
        constexpr int COLUMNS = 256;
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
