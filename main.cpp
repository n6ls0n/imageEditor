#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL3/SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#include <string>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/em_js.h>
#include "./emscripten/emscripten_mainloop_stub.h"
#else
#include "portable-file-dialogs.h"
#include <filesystem>
#endif

// Add these global variables at the top of the file, after the includes
GLuint g_image_texture = 0;
int g_image_width = 0;
int g_image_height = 0;

#ifdef __EMSCRIPTEN__
bool image_loading = false;
const char* load_status = nullptr;

// Function to be called from JavaScript when a file is selected
EM_JS(void, setupFileInput, (), {
    if (window.fileInput) return;  // Only setup once
    
    var fileInput = document.createElement('input');
    fileInput.type = 'file';
    fileInput.accept = 'image/*';
    fileInput.style.display = 'none';
    document.body.appendChild(fileInput);
    window.fileInput = fileInput;
    
    fileInput.onchange = function(event) {
        var file = event.target.files[0];
        if (!file) return;
        
        console.log("File selected:", file.name);
        
        var reader = new FileReader();
        reader.onload = function(e) {
            console.log("File loaded, size:", e.target.result.byteLength);
            var data = new Uint8Array(e.target.result);
            var size = data.length;
            
            try {
                var ptr = Module._malloc(size);
                Module.HEAPU8.set(data, ptr);
                Module._processImageData(ptr, size);
                Module._free(ptr);
            } catch (error) {
                console.error("Error processing image:", error);
                alert("Failed to process image");
            }
        };
        reader.readAsArrayBuffer(file);
    };
    
    window.openFileDialog = function() {
        fileInput.click();
    };
});

// Function to process the image data received from JavaScript
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void processImageData(const unsigned char* data, int length) {
        printf("Processing image data, length: %d\n", length);
        
        if (g_image_texture) {
            glDeleteTextures(1, &g_image_texture);
            g_image_texture = 0;
        }
        
        int width, height, channels;
        unsigned char* image_data = stbi_load_from_memory(data, length, &width, &height, &channels, 4);
        
        if (image_data) {
            printf("Image loaded: %dx%d with %d channels\n", width, height, channels);
            
            GLuint new_texture;
            glGenTextures(1, &new_texture);
            glBindTexture(GL_TEXTURE_2D, new_texture);
            
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
            
            stbi_image_free(image_data);
            
            g_image_texture = new_texture;
            g_image_width = width;
            g_image_height = height;
            printf("Texture created successfully: %u\n", g_image_texture);
        } else {
            printf("Failed to load image: %s\n", stbi_failure_reason());
        }
    }
}
#endif


static void ShowMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

GLuint CreateTextureFromImage(const char* filename, int* out_width, int* out_height) {
    int image_width = 0;
    int image_height = 0;
    int channels = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, &channels, 4);
    if (image_data == nullptr)
        return 0;

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_width = image_width;
    *out_height = image_height;
    return image_texture;
}

// Replace the helper functions with proper guards
#ifndef __EMSCRIPTEN__
void ShowErrorDialog(const char* message) {
    pfd::message("Error", message, pfd::choice::ok, pfd::icon::error).result();
}

bool CanUseNativeFileDialog() {
    return pfd::settings::available();
}
#endif

std::string GetFileExtension(const std::string& filename) {
    size_t pos = filename.find_last_of(".");
    if (pos != std::string::npos)
        return filename.substr(pos);
    return "";
}

bool IsImageFile(const std::string& filename) {
    std::string ext = GetFileExtension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga";
}

// Main code
int main(int, char**)
{
    // Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_MAXIMIZED;
    SDL_Window* window = SDL_CreateWindow("imageEditor", 1280, 720, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    SDL_ShowWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
#ifndef __EMSCRIPTEN__
    bool show_file_browser = false;
    std::string current_path;
    std::vector<std::string> directory_list;
    
    try {
        current_path = std::filesystem::current_path().string();
    } catch (const std::exception& e) {
        fprintf(stderr, "Error getting current path: %s\n", e.what());
        current_path = ".";
    }
#endif

#ifdef __EMSCRIPTEN__
    setupFileInput();
#endif

    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ShowMainMenuBar();

        float menuBarHeight = ImGui::GetFrameHeight();
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight));
        ImGui::SetNextWindowSize(ImVec2(displaySize.x, displaySize.y - menuBarHeight));
        ImGui::Begin("Fullscreen Window", nullptr, 
            ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus);
            
        if (ImGui::Button("Load Image")) {
#ifdef __EMSCRIPTEN__
            emscripten_run_script("window.openFileDialog()");
#else
            if (CanUseNativeFileDialog()) {
                auto selection = pfd::open_file(
                    "Choose an image file",
                    ".",
                    {"Image files", "*.png *.jpg *.jpeg *.bmp *.tga"},
                    pfd::opt::none
                ).result();
                
                if (!selection.empty()) {
                    if (g_image_texture) {
                        glDeleteTextures(1, &g_image_texture);
                        g_image_texture = 0;
                    }
                    g_image_texture = CreateTextureFromImage(selection[0].c_str(), &g_image_width, &g_image_height);
                    if (!g_image_texture) {
                        ShowErrorDialog("Failed to load image!");
                    }
                }
            } else {
                show_file_browser = true;
            }
#endif
        }

#ifdef __EMSCRIPTEN__
        if (image_loading) {
            ImGui::SameLine();
            ImGui::Text("Loading...");
        }
        if (load_status) {
            ImGui::Text("%s", load_status);
        }
#else
        // Add fallback file browser
        if (show_file_browser) {
            ImGui::Begin("File Browser", &show_file_browser);
            
            // Show current path
            ImGui::Text("Current Path: %s", current_path.c_str());
            
            // Up button
            if (ImGui::Button("..")) {
                std::filesystem::path path(current_path);
                if (path.has_parent_path()) {
                    current_path = path.parent_path().string();
                    // Update directory listing
                    directory_list.clear();
                    for (const auto& entry : std::filesystem::directory_iterator(current_path)) {
                        if (entry.is_regular_file() && IsImageFile(entry.path().string())) {
                            directory_list.push_back(entry.path().filename().string());
                        }
                    }
                }
            }
            
            // File list
            for (const auto& filename : directory_list) {
                if (ImGui::Selectable(filename.c_str())) {
                    std::string filepath = (std::filesystem::path(current_path) / filename).string();
                    if (g_image_texture) {
                        glDeleteTextures(1, &g_image_texture);
                        g_image_texture = 0;
                    }
                    g_image_texture = CreateTextureFromImage(filepath.c_str(), &g_image_width, &g_image_height);
                    if (!g_image_texture) {
                        ShowErrorDialog("Failed to load image!");
                    }
                    show_file_browser = false;
                }
            }
            
            ImGui::End();
        }
#endif

        if (g_image_texture) {
            ImVec2 window_size = ImGui::GetContentRegionAvail();
            float scale = std::min(window_size.x / g_image_width, window_size.y / g_image_height);
            ImVec2 image_size(g_image_width * scale, g_image_height * scale);
            
            // Center the image
            ImVec2 cursor_pos = ImGui::GetCursorPos();
            ImVec2 center_pos(
                cursor_pos.x + (window_size.x - image_size.x) * 0.5f,
                cursor_pos.y + (window_size.y - image_size.y) * 0.5f
            );
            ImGui::SetCursorPos(center_pos);
            ImGui::Image((ImTextureID)(uint64_t)g_image_texture, image_size);
        }

        ImGui::End(); // End the fullscreen window

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (g_image_texture)
        glDeleteTextures(1, &g_image_texture);

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
