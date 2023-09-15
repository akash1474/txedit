#include "GLFW/glfw3.h"
#include "MonoLisa-Medium.h"
#include "imgui.h"
#include "pch.h"
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_img.h"
#include "Editor.h"

#define WIDTH 900
#define HEIGHT 600

int width{0};
int height{0};

std::string file_data{0};
Editor editor;
size_t size{0};

void keyboardCallback(GLFWwindow* window,int key, int scancode, int action, int mods);

void draw(GLFWwindow* window,ImGuiIO& io);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    editor.reCalculateBounds=true;
    glViewport(0, 0, width, height);
    draw(window,ImGui::GetIO());
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    for (int i = 0;  i < count;  i++){
        if(std::filesystem::is_directory(paths[i])){
            GL_INFO("Folder: {}",paths[i]);
        }else{
            GL_INFO("File: {}",paths[i]);
            std::ifstream t(paths[i]);
            t.seekg(0, std::ios::end);
            size = t.tellg();
            file_data.resize(size,' ');
            t.seekg(0);
            t.read(&file_data[0], size); 
            editor.SetBuffer(file_data);
        }
    }
}

void draw(GLFWwindow* window,ImGuiIO& io)
{
    static bool isFileLoaded=false;
    if(!isFileLoaded){
            std::ifstream t("D:/Projects/c++/txedit/src/Editor.cpp");
            t.seekg(0, std::ios::end);
            size = t.tellg();
            file_data.resize(size,' ');
            t.seekg(0);
            t.read(&file_data[0], size); 
            editor.SetBuffer(file_data);
            isFileLoaded=true;
    }
    glfwPollEvents();
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    glfwSetDropCallback(window, drop_callback);

    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    dockspace_flags|=ImGuiDockNodeFlags_PassthruCentralNode;
    static ImGuiWindowFlags window_flags=ImGuiWindowFlags_None;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    static bool is_open=true;
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    window_flags |= ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Container",&is_open,window_flags);
    ImGui::PopStyleVar(3);
    static ImGuiID dockspace_id = ImGui::GetID("DDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

    if(ImGui::BeginMenuBar()){
        if(ImGui::BeginMenu("File")){
            ImGui::MenuItem("New File");
            ImGui::MenuItem("Open File");
            ImGui::MenuItem("Open Folder");
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Edit")){
            ImGui::MenuItem("Cut");
            ImGui::MenuItem("Copy");
            ImGui::MenuItem("Paste");
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
    ImGui::ShowDemoWindow();

    ImGui::Begin("Project");
    static int LineSpacing=10.0f;
    if(ImGui::SliderInt("LineSpacing",&LineSpacing,0, 20)){
        editor.setLineSpacing(LineSpacing);
    }
    ImGui::Text("PositionY:%f",ImGui::GetMousePos().y);
    ImGui::End();


    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Editor",0,ImGuiWindowFlags_NoCollapse);
    ImGui::PopStyleVar();

    if(size){
        ImGui::PushFont(io.Fonts->Fonts[1]);
        // ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10));
        // ImGui::InputTextMultiline("##TextEditor", (char*)file_data.c_str(), file_data.size(),{ImGui::GetWindowSize().x,ImGui::GetWindowSize().y-25});
        // ImGui::PopStyleVar();
        editor.render();
        ImGui::PopFont();
    }
    ImGui::End();


    ImGui::Begin("Console");
    ImGui::End();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
}



int main(void){
    GLFWwindow* window;
    #ifdef GL_DEBUG
    OpenGL::Log::Init();
    #endif

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "File Transfer", NULL, NULL);
    glfwSetWindowSizeLimits(window, 330, 500, GLFW_DONT_CARE, GLFW_DONT_CARE);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    GLFWimage images[1]; 
    images[0].pixels = stbi_load_from_memory(logo_img,IM_ARRAYSIZE(logo_img), &images[0].width, &images[0].height, 0, 4); //rgba channels 
    glfwSetWindowIcon(window, 1, images); 
    stbi_image_free(images[0].pixels);

    // Initialize ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();


    GL_INFO("Initializing Fonts");
    io.Fonts->Clear();
    // io.IniFilename=nullptr;
    io.LogFilename=nullptr;


    glfwSwapInterval(1);
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 2.0f;
    style.ItemSpacing.y=6.0f;
    style.ScrollbarRounding=2.0f;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    if (!ImGui_ImplOpenGL2_Init()) GL_ERROR("Failed to initit OpenGL 2");

    // glfwSetKeyCallback(window, keyboardCallback);

    static const ImWchar icons_ranges[] = {ICON_MIN_FA,ICON_MAX_FA,0};
    ImFontConfig icon_config;
    icon_config.MergeMode = true;
    icon_config.PixelSnapH = true;
    icon_config.FontDataOwnedByAtlas=false;

    const int font_data_size = IM_ARRAYSIZE(data_font);
    const int icon_data_size = IM_ARRAYSIZE(data_icon);

    ImFontConfig font_config;
    font_config.FontDataOwnedByAtlas=false;
    io.Fonts->AddFontFromMemoryTTF((void*)data_font, font_data_size,16,&font_config);
    io.Fonts->AddFontFromMemoryTTF((void*)data_icon, icon_data_size,20*2.0f/3.0f,&icon_config,icons_ranges);

    io.Fonts->AddFontFromMemoryTTF((void*)monolisa_medium, IM_ARRAYSIZE(monolisa_medium),12,&font_config);


    StyleColorsDracula();

    while (!glfwWindowShouldClose(window)) {
        draw(window,io);
    }
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}