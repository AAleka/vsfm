#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef linux
const char* SYSTEM = "UNIX";
const char* HOMEDIR = "HOME";
const char* SLASH = "/";
const char* OPEN_APP = "xdg-open ";
#endif

#ifdef _WIN32
#include <Windows.h>
#include <cstdlib>

const char* SYSTEM = "WINDOWS";
const char* HOMEDIR = "HOMEPATH";
const char* SLASH = "\\";
const char* OPEN_APP = "start ";

void HideConsole()
{
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
}

#endif

#include <GL/gl.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <vector>

struct OPTIONS {
    std::string path = getenv(HOMEDIR);
    std::string clicked_path = "";
    std::string copy_path    = "";

    int width  = 820;
    int height = 480;

    char search_path[100] = "";
    char new_name[100]    = "";

    bool is_app_options   = false;
    bool is_menu_options  = false;
    bool is_dir_options   = false;
    bool show_hidden      = false;
    bool is_cut           = false;
    bool is_rename        = false;
    bool is_create_folder = false;
    bool is_create_file   = false;

    ImVec2 mouse_pos;
};

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void main_window(OPTIONS& options, ImGuiWindowFlags file_manager_window_flags, ImGuiWindowFlags options_window_flags);
void left_window(std::string& path, std::string* bookmarks, ImGuiWindowFlags bookmarks_window_flags);

int main() {
    #ifdef _WIN32
        HideConsole();
    #endif

    OPTIONS options;

    std::string bookmarks[6] = {
        options.path,
        options.path + SLASH + "Documents",
        options.path + SLASH + "Downloads",
        options.path + SLASH + "Music",
        options.path + SLASH + "Pictures",
        options.path + SLASH + "Videos"
    };

    assert(glfwInit() && "Could not init GLFW!");

    GLFWwindow *window = glfwCreateWindow(options.width, options.height, "VSFM", nullptr, nullptr);
	
    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "Error: %s\n", description);
        });
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.Fonts->AddFontFromFileTTF("Swansea-q3pd.ttf", 10.0f);
    io.ConfigFlags = ImGuiViewportFlags_IsPlatformMonitor;

    ImGui::StyleColorsDark();
    ImGui::GetStyle().FramePadding.y = 12.0f;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 120");

    // variables
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool show_demo_window = false;

    ImGuiWindowFlags main_window_flags = 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoTitleBar;

    ImGuiWindowFlags bookmarks_window_flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar;

    ImGuiWindowFlags file_manager_window_flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar;

    ImGuiWindowFlags options_window_flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar;
    
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1, 0.1, 0.1, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //ImGui::ShowDemoWindow(&show_demo_window);
	
        // main window
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2{(float) options.width, (float) options.height});
        ImGui::Begin("MainWindow", 0, main_window_flags);
        {
            left_window(options.path, bookmarks, bookmarks_window_flags);
            main_window(options, file_manager_window_flags, options_window_flags);
        }
        
        glfwGetWindowSize(window, &options.width, &options.height);

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    return 0;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void left_window(std::string& path, std::string* bookmarks, ImGuiWindowFlags bookmarks_window_flags) {
    std::string intermediate = "";
    ImGui::SameLine();
    ImGui::SetCursorPosX(0);
    if (ImGui::BeginChild("Bookmarks", ImVec2(200, 0), true, bookmarks_window_flags)) {

        for (int i = 0; i < 6; i++) {
            if (bookmarks[i] != getenv(HOMEDIR)) {
                intermediate = bookmarks[i].substr(bookmarks[i].rfind(SLASH)).erase(0, 1);
                ImGui::Selectable(intermediate.c_str(), false, 0, ImVec2(0, 20.0f));
            }
            else {
                ImGui::Selectable("home", false, 0, ImVec2(0, 20.0f));
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && std::filesystem::is_directory(bookmarks[i]))
                path = bookmarks[i];
        }

        ImGui::EndChild();
    }
}

void main_window(OPTIONS& options, ImGuiWindowFlags file_manager_window_flags, ImGuiWindowFlags options_window_flags) {
    std::string intermediate = "";
    bool is_hidden = false;

    ImGui::SameLine();
    ImGui::SetCursorPosX(200);
    if (ImGui::BeginChild("File Manager", ImVec2(0, 0), true, file_manager_window_flags)) {

        ImGui::BeginDisabled(options.path == getenv(HOMEDIR));
        if (ImGui::Button("..", ImVec2(40.0f, 0))) {
            options.path = options.path.substr(0, options.path.rfind(SLASH));
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0, 30);
        if (ImGui::Button("Options", ImVec2(0, 0))) {
            options.mouse_pos.x = ImGui::GetWindowPos().x + 100;
            options.mouse_pos.y = ImGui::GetWindowPos().y + 50;
            options.is_menu_options = !options.is_menu_options;
        }

        ImGui::SameLine(0, 30);
        ImGui::PushItemWidth(options.width - 400);
        if (ImGui::InputText("##search", options.search_path, 100)) {
            //path = searchPath;
            ImGui::PopItemWidth();
        }

        for (auto& i : std::filesystem::directory_iterator(options.path)) {
            // skip hidden directories and files
            std::stringstream line(i.path().string());

            is_hidden = false;

            while (std::getline(line, intermediate, '/')) {

                if (intermediate.substr(0, 1) == ".")
                    is_hidden = true;
            }

            if (is_hidden && !options.show_hidden)
                continue;

            intermediate = i.path().string().substr(i.path().string().rfind(SLASH)).erase(0, 1);

            std::transform(intermediate.begin(), intermediate.end(), intermediate.begin(), ::tolower);

            for (int i = 0; options.search_path[i] != '\0'; i++)
                options.search_path[i] = tolower(options.search_path[i]);

            // display directories and files
            //intermediate = i.path().string().substr(i.path().string().rfind('/')).erase(0, 1);

            ImGui::Selectable(((std::filesystem::is_directory(i.path()) ? "[D]  " : "[F]  ") + intermediate).c_str(), false, 0, ImVec2(0, 20.0f));

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && std::filesystem::is_directory(i.path().string())) {
                options.path = i.path().string();
                options.is_app_options = false;
            }

            else if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && !std::filesystem::is_directory(i.path().string())) {
                std::system((OPEN_APP + i.path().string()).c_str());
                options.is_app_options = false;
            }
            else if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
                options.is_app_options = true;
                options.is_dir_options = false;
                options.mouse_pos = ImGui::GetMousePos();
                options.clicked_path = i.path().string();
            }
            else if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(1)) {
                options.is_dir_options = true;
                options.is_app_options = false;
                options.mouse_pos = ImGui::GetMousePos();
                options.clicked_path = i.path().string();
            }
        }

        if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(1)) {
            options.is_dir_options = true;
            options.is_app_options = false;
            options.mouse_pos = ImGui::GetMousePos();
            options.clicked_path = options.path;
        }

        if (options.is_app_options) {
            if (options.width - options.mouse_pos.x < 200)
                options.mouse_pos.x -= 200;
            if (options.height - options.mouse_pos.y < 200)
                options.mouse_pos.y -= 200;

            if (ImGui::IsMouseClicked(0) && (ImGui::GetMousePos().x < options.mouse_pos.x || ImGui::GetMousePos().x > options.mouse_pos.x + 200 ||
                ImGui::GetMousePos().y < options.mouse_pos.y || ImGui::GetMousePos().y > options.mouse_pos.y + 200)) {
                options.is_app_options = false;
            }

            ImGui::SetNextWindowPos(options.mouse_pos);
            ImGui::SetNextWindowSize(ImVec2(200, 200));

            ImGui::Begin("**options", &options.is_app_options, options_window_flags);

            ImGui::Selectable("Open", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                if (std::filesystem::is_directory(options.clicked_path))
                    options.path = options.clicked_path;
                else
                    std::system((OPEN_APP + options.clicked_path).c_str());
                options.is_app_options = false;
            }

            ImGui::Selectable("New folder", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                options.is_create_folder = true;
            }

            if (options.is_create_folder) {
                ImGui::InputText("##new name", options.new_name, 100);
                ImGui::SameLine();
                if (ImGui::Button("Ok") && strcmp(options.new_name, "") != 0) {
                    std::system(("mkdir " + options.clicked_path.substr(0, options.clicked_path.rfind(SLASH)) + SLASH + options.new_name).c_str());
                    options.is_app_options = false;
                    options.is_create_folder = false;
                    options.new_name[0] = '\0';
                }
            }

            ImGui::Selectable("New file", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                options.is_create_file = true;
            }

            if (options.is_create_file) {
                ImGui::InputText("##new name", options.new_name, 100);
                ImGui::SameLine();
                if (ImGui::Button("Ok") && strcmp(options.new_name, "") != 0) {
                    std::system(("touch " + options.clicked_path.substr(0, options.clicked_path.rfind(SLASH)) + SLASH + options.new_name).c_str());
                    options.is_app_options = false;
                    options.is_create_file = false;
                    options.new_name[0] = '\0';
                }
            }

            ImGui::Selectable("Cut", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                options.copy_path = options.clicked_path;
                options.is_app_options = false;
                options.is_cut = true;
            }

            ImGui::Selectable("Copy", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                options.copy_path = options.clicked_path;
                options.is_app_options = false;
                options.is_cut = false;
            }

            ImGui::BeginDisabled(options.copy_path.empty() ? true : false);
            ImGui::Selectable("Paste", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && options.copy_path != options.clicked_path) {
                if (std::filesystem::is_directory(options.copy_path)) {
                    std::system(("cp -r " + options.copy_path + " " + options.clicked_path).c_str());
                    if (options.is_cut)
                        std::system(("rm -rf " + options.copy_path).c_str());
                }
                else {
                    std::system(("cp " + options.copy_path + " " + options.clicked_path).c_str());
                    if (options.is_cut)
                        std::system(("rm " + options.copy_path).c_str());
                }

                options.copy_path = "";
                options.is_app_options = false;
                options.is_cut = false;
            }
            ImGui::EndDisabled();

            ImGui::Selectable("Rename", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                options.is_rename = true;
            }

            if (options.is_rename) {
                ImGui::InputText("##new name", options.new_name, 100);
                ImGui::SameLine();
                if (ImGui::Button("Ok") && strcmp(options.new_name, "") != 0) {
                    std::system(("mv " + options.clicked_path + " " + options.clicked_path.substr(0,
                        options.clicked_path.rfind(SLASH)) + SLASH + options.new_name).c_str());
                    options.is_app_options = false;
                    options.is_rename = false;
                    options.new_name[0] = '\0';
                }
            }

            ImGui::Selectable("Compress", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                //                std::cout << ("tar czf " + clicked_path + ".tar.gz " + clicked_path).c_str() << '\n';
                std::system(("tar czf " + options.clicked_path + ".tar.gz " + options.clicked_path).c_str());

                options.is_app_options = false;
            }

            ImGui::Selectable("Remove", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                if (std::filesystem::is_directory(options.clicked_path))
                    std::system(("gio trash " + options.clicked_path).c_str());
                else
                    std::system(("gio trash " + options.clicked_path).c_str());

                options.is_app_options = false;
            }

            ImGui::Selectable("Properties", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                if (std::filesystem::is_directory(options.clicked_path))

                    options.is_app_options = false;
            }

            ImGui::End();
        }

        if (options.is_dir_options) {
            if (options.width - options.mouse_pos.x < 200)
                options.mouse_pos.x -= 200;
            if (options.height - options.mouse_pos.y < 200)
                options.mouse_pos.y -= 200;

            if (ImGui::IsMouseClicked(0) && (ImGui::GetMousePos().x < options.mouse_pos.x || ImGui::GetMousePos().x > options.mouse_pos.x + 200 ||
                ImGui::GetMousePos().y < options.mouse_pos.y || ImGui::GetMousePos().y > options.mouse_pos.y + 200)) {
                options.is_dir_options = false;
            }

            ImGui::SetNextWindowPos(options.mouse_pos);
            ImGui::SetNextWindowSize(ImVec2(200, 200));

            ImGui::Begin("**options", &options.is_app_options, options_window_flags);

            ImGui::Selectable("New folder", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                options.is_create_folder = true;
            }

            if (options.is_create_folder) {
                ImGui::InputText("##new name", options.new_name, 100);
                ImGui::SameLine();
                if (ImGui::Button("Ok") && strcmp(options.new_name, "") != 0) {
                    std::system(("mkdir " + options.clicked_path + SLASH + options.new_name).c_str());
                    options.is_dir_options = false;
                    options.is_create_folder = false;
                    options.new_name[0] = '\0';

                }
            }

            ImGui::Selectable("New file", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                options.is_create_file = true;
            }

            if (options.is_create_file) {
                ImGui::InputText("##new name", options.new_name, 100);
                ImGui::SameLine();
                if (ImGui::Button("Ok") && strcmp(options.new_name, "") != 0) {
                    std::system(("touch " + options.clicked_path + SLASH + options.new_name).c_str());
                    options.is_dir_options = false;
                    options.is_create_file = false;
                    options.new_name[0] = '\0';
                }
            }

            ImGui::BeginDisabled(options.copy_path.empty() ? true : false);
            ImGui::Selectable("Paste", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && options.copy_path != options.clicked_path) {
                if (std::filesystem::is_directory(options.copy_path)) {
                    std::system(("cp -r " + options.copy_path + " " + options.clicked_path).c_str());
                    if (options.is_cut)
                        std::system(("rm -rf " + options.copy_path).c_str());
                }
                else {
                    std::system(("cp " + options.copy_path + " " + options.clicked_path).c_str());
                    if (options.is_cut)
                        std::system(("rm " + options.copy_path).c_str());
                }

                options.copy_path = "";
                options.is_dir_options = false;
                options.is_cut = false;
            }
            ImGui::EndDisabled();

            ImGui::Selectable("Properties", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                if (std::filesystem::is_directory(options.clicked_path))
                    options.is_dir_options = false;
            }

            ImGui::End();
        }

        if (options.is_menu_options) {
            if (options.width - options.mouse_pos.x < 210)
                options.mouse_pos.x -= 200;
            if (options.height - options.mouse_pos.y < 210)
                options.mouse_pos.y -= 200;
            ImGui::SetNextWindowPos(options.mouse_pos);
            ImGui::SetNextWindowSize(ImVec2(200, 200));

            ImGui::Begin("**menuoptions", &options.is_app_options,
                ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoCollapse
                | ImGuiWindowFlags_NoTitleBar
            );

            ImGui::Checkbox("Show hidden", &options.show_hidden);


            ImGui::End();
        }

        ImGui::EndChild();
    }
}