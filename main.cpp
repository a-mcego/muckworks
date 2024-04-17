#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <iostream>
#include "xroads/xroads.h"

using Xr::u64, Xr::u32, Xr::u16, Xr::u8, Xr::i64, Xr::i32, Xr::i16, Xr::i8, Xr::f64, Xr::f32;
const Xr::C2i WINDOW_SIZE{800,480};
using std::cout, std::endl;

struct Pixel
{
    enum struct TYPE : unsigned char //underlying type subject to change
    {
        VOID,
        STONE,
        WATER,
        SAND,

        N
    } type=TYPE::VOID;

    bool IsEmpty()
    {
        return type == TYPE::VOID;
    }

    u32 Color() // ARGB format
    {
        if (type == TYPE::VOID)
            return 0xFF000000;
        if (type == TYPE::SAND)
            return 0xFFF0C000;
        if (type == TYPE::WATER)
            return 0xFF2040FF;
        Xr::Kill("Unknwon type.");
    }
};

struct World
{
    Xr::Vector2D<Pixel> pixels{WINDOW_SIZE, Pixel()};

    void Update()
    {
        pixels(WINDOW_SIZE.x/3*2, 0) = Pixel{Pixel::TYPE::SAND};
        pixels(WINDOW_SIZE.x/3, 0) = Pixel{Pixel::TYPE::WATER};

        Xr::Vector2D<u8> updated(pixels.size(), 0);
        for(int y=0; y<pixels.Y(); ++y)
        {
            for(int x=0; x<pixels.X(); ++x)
            {
                if (updated(x,y))
                    continue;

                Pixel& p = pixels(x,y);
                if (p.type == Pixel::TYPE::VOID)
                {
                    ;//do nothing
                }
                else if (p.type == Pixel::TYPE::SAND || p.type == Pixel::TYPE::WATER)
                {
                    Xr::C2i newplace(-1,-1);

                    Xr::C2i tryplace(x,y+1);
                    Xr::C2i offset(int(Xr::Random(0,1)*2)-1,0); //try left/right randomly

                    std::array<Xr::C2i,5> tryplaces =
                    {
                        tryplace, tryplace+offset, tryplace-offset, Xr::C2i(-1,-1), Xr::C2i(-1,-1)
                    };

                    if (p.type == Pixel::TYPE::WATER)
                    {
                        tryplaces[3] = tryplaces[1]-Xr::C2i(0,1);
                        tryplaces[4] = tryplaces[2]-Xr::C2i(0,1);
                    }

                    bool found = false;

                    for(auto& pl: tryplaces)
                    {
                        if (!pixels.in_bounds(pl))
                            continue;
                        if (pixels(pl).IsEmpty())
                        {
                            newplace = pl;
                            found = true;
                            break;
                        }
                    }
                    if (found)
                    {
                        std::swap(p, pixels(newplace));
                        //pixels(newplace) = p;
                        //p.type = Pixel::TYPE::VOID;
                        updated(newplace) = 1;
                    }
                }
            }
        }

    }
};

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("SDL Pixel Access", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_SIZE.x, WINDOW_SIZE.y, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_SIZE.x, WINDOW_SIZE.y);

    bool running = true;
    SDL_Event event;

    World world;

    bool runSimulation = false;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_f)
                {
                    runSimulation = true;
                }
            }
        }

        if (runSimulation)
        {
            world.Update();
        }

        void* pixels = nullptr;
        int pitch=0;
        SDL_LockTexture(texture, NULL, &pixels, &pitch);

        // Example: Fill the screen with red
        u32* pixel_ptr = static_cast<u32*>(pixels);
        for (int y = 0; y < WINDOW_SIZE.y; ++y)
        {
            for (int x = 0; x < WINDOW_SIZE.x; ++x)
            {
                u32 pos = y * (pitch / sizeof(u32)) + x;
                pixel_ptr[pos] = world.pixels(x,y).Color(); // ARGB format
            }
        }

        SDL_UnlockTexture(texture);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

