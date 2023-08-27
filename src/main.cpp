#include "pch.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_img.h"

#define WIDTH 400
#define HEIGHT 600

int width{0};
int height{0};

void draw(GLFWwindow* window)
{
    glfwGetWindowSize(window, &width, &height);
    // Rendering code goes here
    glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // Render Other Stuff


    // Render Imgui Stuff


    // End of render
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    draw(window);
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
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    if (!ImGui_ImplOpenGL2_Init()) GL_ERROR("Failed to initit OpenGL 2");


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

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        ImGui::ShowDemoWindow();

        ImGui::Begin("Project");
        ImGui::End();


        ImGui::Begin("Editor");
        ImGui::End();


        ImGui::Begin("Console");
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
