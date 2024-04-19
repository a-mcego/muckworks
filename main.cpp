#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <iostream>
#include <map>
using std::cout, std::endl;

#include "xroads/xroads.h"
using Xr::u64, Xr::u32, Xr::u16, Xr::u8, Xr::i64, Xr::i32, Xr::i16, Xr::i8, Xr::f64, Xr::f32;
const Xr::C2i WINDOW_SIZE{400,180};

const u32 mult = 0x9E3779B9U;

struct Material
{
    Xr::Sm16 name;
    u32 color{}; // RGB format
    u32 density{}; // 24:8 fixed point. air is around 0x100 and water around 0x3E800 (1000 * 256)
    enum struct STATE
    {
        SOLID,
        LIQUID,
        GASEOUS,
        N
    } state{STATE::SOLID};
};

u32 operator""_density(u64 data)
{
    return u32(data*0x100);
}

//dry sand: 1631 kg/m^3
//wet sand: 2039 kg/m^3
//granite:  2691 kg/m^3

const std::map<Xr::Sm16, Material> materials =
{
    {"air"_sm16, {"air"_sm16, 0x000000, 1_density, Material::STATE::GASEOUS}},
    {"water"_sm16, {"water"_sm16, 0x2040FF, 1000_density, Material::STATE::LIQUID}},
    {"sand"_sm16, {"sand"_sm16, 0xF0C000, 1631_density, Material::STATE::SOLID}},
    {"stone"_sm16, {"stone"_sm16, 0x9090A0, 2691_density, Material::STATE::SOLID}}, //say it's granite
};

struct Pixel
{
    Xr::Sm16 type{"air"_sm16};
    Material material;

    Pixel()
    {
        UpdateMaterial();
    }

    Pixel(Xr::Sm16 type_)
    {
        type = type_;
        UpdateMaterial();
    }

    void UpdateMaterial()
    {
        auto iter = materials.find(type);
        if (iter == materials.end())
        {
            Xr::Kill("Material not found. Sadge.");
        }
        material = iter->second;
    }

    const Material& GetMaterial()
    {
        return material;
    }

    u32 Color() // ARGB format
    {
        return GetMaterial().color | 0xFF000000U;
    }
};

struct World
{
    Xr::Vector2D<Pixel> pixels{WINDOW_SIZE, Pixel()};
    u64 tick{};

    void Update(bool runSimulation)
    {
        ++tick;
        //spawn some sand and water for illustration purposes

        if (runSimulation)
        {
            if (tick%1024<384)
            {
                pixels(WINDOW_SIZE.x/3*2, 0) = Pixel{"sand"_sm16};
                pixels(WINDOW_SIZE.x/3, 0) = Pixel{"water"_sm16};
                pixels(WINDOW_SIZE.x/3+4, 0) = Pixel{"water"_sm16};
                pixels(WINDOW_SIZE.x/3-4, 0) = Pixel{"water"_sm16};
                //pixels(WINDOW_SIZE.x/3+8, 0) = Pixel{"water"_sm16};
                //pixels(WINDOW_SIZE.x/3-8, 0) = Pixel{"water"_sm16};
            }
            else
            {
                pixels(WINDOW_SIZE.x/3, 0) = Pixel{"sand"_sm16};
                pixels(WINDOW_SIZE.x/3*2, 0) = Pixel{"water"_sm16};
                pixels(WINDOW_SIZE.x/3*2-4, 0) = Pixel{"water"_sm16};
                pixels(WINDOW_SIZE.x/3*2+4, 0) = Pixel{"water"_sm16};
                //pixels(WINDOW_SIZE.x/3*2-8, 0) = Pixel{"water"_sm16};
                //pixels(WINDOW_SIZE.x/3*2+8, 0) = Pixel{"water"_sm16};
            }
        }

        struct UpdateInfo
        {
            Xr::C2i position{-1,-1};
            int counter{0};

            void Push(Xr::C2i new_position)
            {
                if (Xr::Random(0,counter) == 0)
                {
                    position = new_position;
                }
                counter += 1;
            }
        };
        Xr::Vector2D<UpdateInfo> updated(pixels.size(), UpdateInfo{});
        for(int counter=0; counter<WINDOW_SIZE.x*WINDOW_SIZE.y; ++counter)
        {
            int x = counter%WINDOW_SIZE.x;
            int y = counter/WINDOW_SIZE.x;

            Xr::C2i currplace{x,y};

            Pixel& p = pixels(currplace);
            const Material& m = p.GetMaterial();

            Xr::C2i newplace(-1,-1);
            Xr::C2i tryplace(x,y+1);
            Xr::C2i offset(int(Xr::Random(0,1)*2)-1,0); //try left/right randomly

            std::array<Xr::C2i,5> tryplaces =
            {
                tryplace, tryplace+offset, tryplace-offset, Xr::C2i(-1,-1), Xr::C2i(-1,-1)
            };

            //if liquid/gaseous, try to move directly left/right as well
            //TODO: make this be based on some more fundamental idea than just the material state
            if (m.state == Material::STATE::LIQUID || m.state == Material::STATE::GASEOUS)
            {
                tryplaces[3] = tryplaces[1]-Xr::C2i(0,1);
                tryplaces[4] = tryplaces[2]-Xr::C2i(0,1);
            }

            bool found = false;

            for(auto pl: tryplaces)
            {
                if (!pixels.in_bounds(pl))
                    continue;
                //is there something else we could do here instead of a density check?
                //or is there something *more* we could do?
                if (m.density > pixels(pl).GetMaterial().density)
                {
                    newplace = pl;
                    found = true;
                    break;
                }
            }
            if (found)
            {
                updated(newplace).Push(currplace);
            }
        }
        for(int counter=0; counter<WINDOW_SIZE.x*WINDOW_SIZE.y; ++counter)
        {
            int x = counter%WINDOW_SIZE.x;
            int y = counter/WINDOW_SIZE.x;

            Xr::C2i newplace{x,y};

            auto& update = updated(newplace);
            if (update.counter > 0)
            {
                std::swap(pixels(update.position), pixels(newplace));
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

    // Variables for FPS calculation
    u32 lastTime = SDL_GetTicks(), currentTime;
    u32 frameCount = 0;

    World world;

    const u32 FRAMETIME = 1000.0f/60.0f;
    u32 lastFrame = SDL_GetTicks();

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
                    runSimulation = !runSimulation;
                }
            }
        }

        if (SDL_GetTicks()-lastFrame >= FRAMETIME)
        {
            world.Update(runSimulation);
            lastFrame += FRAMETIME;
        }

        void* pixels = nullptr;
        int pitch=0;
        SDL_LockTexture(texture, NULL, &pixels, &pitch);

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

        // FPS calculation and display
        frameCount += 1;
        currentTime = SDL_GetTicks();
        if (currentTime - lastTime >= 1000)
        {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime += 1000;
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
