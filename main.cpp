#include <iostream>
#include <random>
#include <algorithm>
#include <iterator>
#include <chrono>

#include "miulaw.h"
#include "filter_coeff.h"

#include <SDL2/SDL.h>
#include "sdl_context.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

const int SCALE_FACTOR = 4;

struct Image {
    struct Pixel {
        unsigned char r, g, b, a;
    };

    std::vector<Pixel> data;
    int width = 0, height = 0;

    Pixel& operator()(int x, int y) {
        x = (x + width) % width;
        y = (y + height) % height;
        return data[y * width + x];
    }

    const Pixel& operator()(int x, int y) const {
        x = (x + width) % width;
        y = (y + height) % height;
        return data[y * width + x];
    }

    auto get_it(int x, int y) {
        return data.begin() + (y * width + x);
    }

    void resize(int width_, int height_) {
        width = width_;
        height = height_;
        data.resize(width * height, Pixel{0, 0, 0, 0});
    }
};

struct ivec2 {
    int x, y;
};

struct PatternData {
    struct Circle {
        ivec2 last_position = {0, 0};
        ivec2 position = {0, 0};
        ivec2 desired_position = {0, 0};
    };

    std::vector<Circle> circles;
    std::mt19937 random_engine;
    int circle_radius;
};

void pattern_data_iteration(Image& image, PatternData& pattern_data) {
    std::uniform_int_distribution<int> width_distribution{0, image.width};
    std::uniform_int_distribution<int> height_distribution{0, image.height};

    for(auto& c: pattern_data.circles) {
        if(c.last_position.x == c.position.x && c.last_position.y == c.position.y) {
            c.desired_position = {
                width_distribution(pattern_data.random_engine),
                height_distribution(pattern_data.random_engine)
            };
        }

        c.last_position = c.position;
        c.position.x = 980 * c.position.x / 1000 + 20 * c.desired_position.x / 1000;
        c.position.y = 980 * c.position.y / 1000 + 20 * c.desired_position.y / 1000;

        for(int y = c.position.y - pattern_data.circle_radius; y <= c.position.y + pattern_data.circle_radius; y++) {
            for(int x = c.position.x - pattern_data.circle_radius; x <= c.position.x + pattern_data.circle_radius; x++) {
                if(
                        (x - c.position.x) * (x - c.position.x)
                        + (y - c.position.y) * (y - c.position.y)
                        <= pattern_data.circle_radius * pattern_data.circle_radius) {
                    image(x,y) = Image::Pixel{255, 255, 255, 255};
                }
            }
        }
    }
}

void game_of_life_iteration(Image& out, const Image& in) {
    for(int y = 0; y < in.height; y++) {
        for(int x = 0; x < in.width; x++) {
            int neighbours = -(in(x, y).a == 255);

            for(int dx = -1; dx <= 1; dx++) {
                for(int dy = -1; dy <= 1; dy++) {
                    neighbours += (in(x + dx, y + dy).a == 255);
                }
            }

            bool alive = neighbours == 3 || (neighbours == 2 && (in(x, y).a == 255));

            if(alive) {
                out(x, y) = Image::Pixel{255, 255, 255, 255};
            } else {
                int decay_rate = std::max(int(in(x, y).a) / 20, 1);
                out(x, y).a = std::max(int(in(x, y).a) - decay_rate, 0);
            }

            if(out(x, y).a == 255) {
                out(x, y) = Image::Pixel{255, 255, 255, 255};
            } else if(out(x, y).a == 0) {
                out(x, y) = Image::Pixel{0, 0, 0, 0};
            } else {
                out(x, y) = Image::Pixel{152, 3, 252, out(x, y).a};
            }
        }
    }
}

void pixel_sort(Image& image) {
    int threshold = 100;

    for(int y = 0; y < image.height; y++) {
        int last_x = -1;

        for(int x = 0; x < image.width; x++) {
            if(image(x, y).a <= threshold) {
                if(last_x == -1) {
                    last_x = x;
                } else {
                    std::sort(
                            image.get_it(last_x, y),
                            image.get_it(x, y),
                            [](const auto& lhs, const auto& rhs) {
                                return lhs.a > rhs.a;
                            });
                    last_x = -1;
                }
            }
        }
    }
}

void filter_eq(Image& image) {
    for(auto& pixel: image.data) {
        pixel.r = int(pixel.r) * pixel.a / 255;
        pixel.g = int(pixel.g) * pixel.a / 255;
        pixel.b = int(pixel.b) * pixel.a / 255;
        pixel.a = 255;
    }

    std::vector<float> red_img, green_img, blue_img;

    red_img.resize(image.data.size(), 0.f);
    green_img.resize(image.data.size(), 0.f);
    blue_img.resize(image.data.size(), 0.f);

    for(int i = 0; i < image.data.size(); i++) {
        float r = 0.f, g = 0.f, b = 0.f;

        red_img[i] = mulaw_decode(image.data[i].r);
        green_img[i] = mulaw_decode(image.data[i].g);
        blue_img[i] = mulaw_decode(image.data[i].b);

        for(int j = i, k = 0; k < filter_coeff_size && j >= 0; j--, k++) {
            r += filter_coeff[k] * red_img[j];
            g += filter_coeff[k] * green_img[j];
            b += filter_coeff[k] * blue_img[j];
        }

        image.data[i].r = mulaw_encode(r);
        image.data[i].g = mulaw_encode(g);
        image.data[i].b = mulaw_encode(b);
    }
}

int main()
{
    auto main_loop = [](){
        static int last_width = -1, last_height = -1;
        static SDL_Renderer* render = SDL_CreateRenderer(
                SDLContext::get().window(),
                -1,
                SDL_RENDERER_SOFTWARE
            );
        static SDL_Texture* texture = nullptr;
        static Image game_of_life_board;
        static Image final;
        static PatternData pattern_data {
            .random_engine = std::mt19937{}
        };

        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            if(e.type == SDL_QUIT)
                SDLContext::get().set_should_close(true);
        }

        int w, h;
        SDL_GetWindowSize(SDLContext::get().window(), &w, &h);

        if(w != last_width && h != last_height) {
            last_width = w;
            last_height = h;

            if(texture) {
                SDL_DestroyTexture(texture);
            }

            texture = SDL_CreateTexture(
                    render,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                    SDL_PIXELFORMAT_RGBA8888,
#else
                    SDL_PIXELFORMAT_ABGR8888,
#endif
                    SDL_TEXTUREACCESS_STREAMING,
                    w / SCALE_FACTOR,
                    h / SCALE_FACTOR
            );

            game_of_life_board.resize(w / SCALE_FACTOR, h / SCALE_FACTOR);
            final.resize(w / SCALE_FACTOR, h / SCALE_FACTOR);

            pattern_data.circles.resize(5, PatternData::Circle{
                    {game_of_life_board.width / 2, game_of_life_board.height / 2},
                    {game_of_life_board.width / 2, game_of_life_board.height / 2},
                    {game_of_life_board.width / 2, game_of_life_board.height / 2}
            });

            pattern_data.circle_radius = std::max(std::max(game_of_life_board.width, game_of_life_board.height) / 28, 1);
        }

        pattern_data_iteration(game_of_life_board, pattern_data);
        game_of_life_iteration(final, game_of_life_board);
        pixel_sort(final);
        game_of_life_board = final;
        filter_eq(final);

        void* pixels;
        int pitch;
        SDL_LockTexture(texture, nullptr, &pixels, &pitch);
        std::copy(
                reinterpret_cast<uint8_t*>(final.data.data()),
                reinterpret_cast<uint8_t*>(final.data.data() + final.width * final.height),
                static_cast<uint8_t*>(pixels)
        );
        SDL_UnlockTexture(texture);

        SDL_RenderClear(render);
        SDL_RenderCopy(
            render,
            texture,
            nullptr,
            nullptr
            );

        SDL_RenderPresent(render);
    };

    #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(main_loop, 0, 1);
    #else
        while(!SDLContext::get().should_close())
        {
            main_loop();
            SDL_Delay(20);
        }
    #endif

    return 0;
}
