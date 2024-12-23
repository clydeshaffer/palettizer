#include "SDL_inc.h"

#include "imgui.h"
#include "implot.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_sdlrenderer2.h"

SDL_Window* mainWindow = NULL;
SDL_Renderer* mainRenderer = NULL;

ImGuiContext* main_imgui_ctx;
ImPlotContext* main_implot_ctx;

SDL_Event e;
bool running = true;

#ifndef WINDOW_TITLE
#define WINDOW_TITLE "The Palettizer"
#endif

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

void mainloop_application() {
    if(ImGui::BeginMainMenuBar()) {
        if(ImGui::BeginMenu("File")) {
            if(ImGui::MenuItem("Open")) {

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
    }
}

void mainloop_wrapper() {
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