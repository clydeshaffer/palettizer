#include "SDL_inc.h"

#include "imgui.h"
#include "implot.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_sdlrenderer2.h"
#include "tinyfd/tinyfiledialogs.h"
#include "bmpmini.hpp"

#include "colortable.h"

SDL_Window* mainWindow = NULL;
SDL_Renderer* mainRenderer = NULL;
SDL_Texture* mainWindowTexture = NULL;
SDL_Texture* loaded_image_texture = NULL;
SDL_Texture* dithered_texture = NULL;

ImGuiContext* main_imgui_ctx;
ImPlotContext* main_implot_ctx;

using namespace image;
BMPMini* mainBmp;
SDL_Rect bmpRect;
double viewScale = 1;
#define MINSCALE 0.1
#define MAXSCALE 64.0

float brightness = 1, contrast = 1, hue = 0, saturation = 1;
float ditherAmount = 1;

SDL_Event e;
bool running = true;

#ifndef WINDOW_TITLE
#define WINDOW_TITLE "The Palettizer"
#endif

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define CLAMP8(x) if(x < 0) x = 0; if(x > 255) x = 255;

struct BGRSigned {
    int8_t b,g,r;

    BGRSigned operator*(float const &scalar) {
        BGRSigned scaled;
        scaled.b = (int) ((float) b * scalar);
        scaled.g = (int) ((float) g * scalar);
        scaled.r = (int) ((float) r * scalar);
        return scaled;
    }
};

struct BGR {
    uint8_t b, g, r;

    BGR operator+(BGRSigned const &color) {
        int16_t bs = b + color.b;
        int16_t gs = g + color.g;
        int16_t rs = r + color.r;

        CLAMP8(bs);
        CLAMP8(gs);
        CLAMP8(rs);

        BGR sum;
        sum.b = bs;
        sum.g = gs;
        sum.r = rs;
        return sum;
    }

    BGRSigned operator-(BGR const &color) {
        BGRSigned dif;
        dif.b = b - color.b;
        dif.g = g - color.g;
        dif.r = r - color.r;
        return dif;
    }

    BGR operator*(float const &scalar) {
        BGR scaled;
        scaled.b = (int) ((float) b * scalar);
        scaled.g = (int) ((float) g * scalar);
        scaled.r = (int) ((float) r * scalar);
        return scaled;
    }
};

struct SDL_Rect new_rect(int x, int y, int w, int h)
{
struct SDL_Rect r = {.x = x, .y = y, .w = w, .h = h};
return r;
}

#define SQUARE(x) ((x) * (x))

float color_dist(BGR c, RGB p) {
    return SQUARE((((float) c.r) - ((float) p.r))) +
    SQUARE((((float) c.g) - ((float) p.g))) +
    SQUARE((((float) c.b) - ((float) p.b)));
}

BGR nearest_palette_color(BGR c) {
    int nearest_ind = -1;
    float nearest_dist = std::numeric_limits<float>::infinity();
    for(int i = 0; i < 256; ++i) {
        float d = color_dist(c, gt_palette[i]);
        if(d < nearest_dist) {
            nearest_dist = d;
            nearest_ind = i;
        }
    }
    BGR result;
    result.r = gt_palette[nearest_ind].r;
    result.g = gt_palette[nearest_ind].g;
    result.b = gt_palette[nearest_ind].b;
    return result;
}

void update_dithered_texture() {
    if(!loaded_image_texture) return;
    if(!dithered_texture) return;

    BGR* original_pixels = (BGR*) mainBmp->get().data;
    BGR* dithered_pixels;
    BGR* dithered_pixels_start;
    int pitch;
    SDL_LockTexture(dithered_texture, NULL, (void**) &dithered_pixels, &pitch);
    dithered_pixels_start = dithered_pixels;

    for(int row = 0; row < bmpRect.h; row++) {
        for(int col = 0; col < bmpRect.w; col++) {
            *dithered_pixels = (*original_pixels) * brightness;
            ++dithered_pixels;
            ++original_pixels;
        }
    }

    
    dithered_pixels = dithered_pixels_start;
    for(int row = 0; row < bmpRect.h; row++) {
        for(int col = 0; col < bmpRect.w; col++) {
            BGR oldValue = *dithered_pixels;
            BGR newValue = nearest_palette_color(*dithered_pixels);
            *dithered_pixels = newValue;

            BGRSigned error = oldValue - newValue;
            if(col < (bmpRect.w)) {
                *(dithered_pixels+1) = *(dithered_pixels+1) + (error * ditherAmount);
            }
            ++dithered_pixels;
        }
    }
    SDL_UnlockTexture(dithered_texture);
}

void mainloop_application() {
    int menuHeight = 0;
    if(ImGui::BeginMainMenuBar()) {
        menuHeight = ImGui::GetWindowSize().y;
        if(ImGui::BeginMenu("File")) {
            if(ImGui::MenuItem("Open")) {
                char const * lFilterPatterns[1] = {"*.bmp"};
                char* openFilePath = tinyfd_openFileDialog(
                    "Open an image file",
                    "",
                    1,
                    lFilterPatterns,
                    "Bitmap Image",
                    0);
                if(openFilePath != NULL) {
                    if(loaded_image_texture != NULL) {
                        SDL_DestroyTexture(loaded_image_texture);
                        delete mainBmp;
                    }
                    mainBmp = new BMPMini();
                    mainBmp->read(openFilePath);
                    auto img = mainBmp->get();
                    img.channels = 1;
                    loaded_image_texture = SDL_CreateTexture(mainRenderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STATIC, img.width, img.height);
                    dithered_texture = SDL_CreateTexture(mainRenderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, img.width, img.height);
                    SDL_UpdateTexture(loaded_image_texture, NULL, img.data, img.width * 3);
                    bmpRect.x = 0;
                    bmpRect.y = menuHeight;  
                    bmpRect.w = img.width;
                    bmpRect.h = img.height;
                    update_dithered_texture();
                }
            }
            if(ImGui::MenuItem("Save")) {

            }
            if(ImGui::MenuItem("Exit")) {
                running = false;
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Edit")) {
            if(ImGui::MenuItem("Copy")) {

            }
            if(ImGui::MenuItem("Cut")) {
                
            }
            if(ImGui::MenuItem("Paste")) {
                
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("View")) {
            if(ImGui::MenuItem("Show Grid Lines")) {

            }
            if(ImGui::MenuItem("Enable Blim Flargs")) {
                
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();

        if(ImGui::BeginChild("Options")) {
            if(ImGui::Button("Do something")) {
                update_dithered_texture();
            }
            if(ImGui::SliderFloat("Brightness", &brightness, 0, 2)) {
                update_dithered_texture();
            }
            ImGui::SliderFloat("Contrast", &contrast, 0, 2);
            ImGui::SliderFloat("Hue", &hue, -180, 180);
            if(ImGui::SliderFloat("Dither%", &ditherAmount, 0, 1)) {
                update_dithered_texture();
            }
            ImGui::EndChild();
        }
    }
    while( SDL_PollEvent( &e ) != 0 ) {
        if(SDL_GetMouseFocus() == mainWindow) {
            ImGui::SetCurrentContext(main_imgui_ctx);
            ImPlot::SetCurrentContext(main_implot_ctx);
            ImGui_ImplSDL2_ProcessEvent(&e);
        }
        if(ImGui::GetIO().WantCaptureKeyboard && ((e.type == SDL_KEYDOWN) || (e.type == SDL_KEYUP))) {
            continue;
        }
        if( e.type == SDL_QUIT )
        {
            running = false;
        }

        if(e.type == SDL_MOUSEWHEEL) {
            viewScale += e.wheel.preciseY / 10;
            if(viewScale < MINSCALE) viewScale = MINSCALE;
            if(viewScale > MAXSCALE) viewScale = MAXSCALE;
        }
    }

    if(loaded_image_texture != NULL) {
        SDL_Rect transformedRect = bmpRect;
        transformedRect.w = (int) (((double) transformedRect.w) * viewScale);
        transformedRect.h = (int) (((double) transformedRect.h) * viewScale);
        SDL_RenderCopy( mainRenderer, loaded_image_texture, NULL, &transformedRect);
        transformedRect.x += transformedRect.w + 16;
        SDL_RenderCopy( mainRenderer, dithered_texture, NULL, &transformedRect);
    }
}

void mainloop_wrapper() {
    SDL_RenderClear(mainRenderer);
    ImGui::SetCurrentContext(main_imgui_ctx);
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

    mainloop_application();

    ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(mainRenderer);
    SDL_UpdateWindowSurface(mainWindow);

    if(!running) {
        #ifdef WASM_BUILD
                emscripten_cancel_main_loop();
        #else
                ImPlot::DestroyContext(main_implot_ctx);
                ImGui::DestroyContext(main_imgui_ctx);
        #endif
                printf("Finished running\n");
                
                SDL_DestroyRenderer(mainRenderer);
                SDL_DestroyWindow(mainWindow);
                SDL_Quit();
            }
}

int main(int argC, char* argV[]) {
    SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);
    mainWindow = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	mainRenderer = SDL_CreateRenderer(mainWindow, -1, SDL_RENDERER_ACCELERATED);
    main_imgui_ctx = ImGui::CreateContext();
	main_implot_ctx = ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigViewportsNoAutoMerge = true;
	io.IniFilename = NULL;
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForSDLRenderer(mainWindow, mainRenderer);
	ImGui_ImplSDLRenderer2_Init(mainRenderer);

    SDL_RaiseWindow(mainWindow);
	while(running) {
		mainloop_wrapper();
	}

    return 0;
}