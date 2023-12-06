#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

#ifdef linux
    const char *HOMEDIR = "HOME";
#endif

#ifdef _WIN32
    const char *HOMEDIR = "HOMEPATH";
#endif

void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// button size, dir_count, num_buttons_line, max_button_space, button_space
void main_window(int width, int height, std::string &path, char * search_path,
                 bool &is_app_options, bool & is_menu_options, ImVec2 & mouse_pos, bool & show_hidden, std::string &clicked_path,
                 std::string &copy_path, bool &is_cut, char * new_name, bool &is_rename, bool &is_create_folder, bool &is_create_file,
                 bool &is_dir_options) {
    std::string intermediate = "";
    bool is_hidden = false;

    ImGui::SameLine();
    ImGui::SetCursorPosX(200);
    if (ImGui::BeginChild("File Manager", ImVec2(0, 0), true,
                      ImGuiWindowFlags_NoResize
                      | ImGuiWindowFlags_NoCollapse
                      | ImGuiWindowFlags_NoMove
                      | ImGuiWindowFlags_NoTitleBar)) {

        ImGui::BeginDisabled(path == getenv(HOMEDIR));
        if (ImGui::Button("..", ImVec2(40.0f, 0))) {
            path = path.substr(0, path.rfind("/"));
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0, 30);
        if (ImGui::Button("Options", ImVec2(0, 0))) {
            mouse_pos.x = ImGui::GetWindowPos().x + 100;
            mouse_pos.y = ImGui::GetWindowPos().y + 50;
            is_menu_options = !is_menu_options;
        }

        ImGui::SameLine(0, 30);
        ImGui::PushItemWidth(width - 400);
        if (ImGui::InputText("##search", search_path, 100)) {
            //path = searchPath;
            ImGui::PopItemWidth();
        }

        for (auto& i : std::filesystem::directory_iterator(path)) {
            // skip hidden directories and files
            std::stringstream line(i.path().string());

            is_hidden = false;

            while (std::getline(line, intermediate, '/')) {
                if (intermediate.substr(0, 1) == ".")
                    is_hidden = true;
            }

            if (is_hidden && !show_hidden)
                continue;

            intermediate = i.path().string().substr(i.path().string().rfind('/')).erase(0, 1);
            std::transform(intermediate.begin(), intermediate.end(), intermediate.begin(), ::tolower);

            for(int i = 0; search_path[i] != '\0'; i++)
                search_path[i] = tolower(search_path[i]);

            // display directories and files
            intermediate = i.path().string().substr(i.path().string().rfind('/')).erase(0, 1);

            ImGui::Selectable(((std::filesystem::is_directory(i.path()) ? "[D]  " : "[F]  ") + intermediate).c_str(), false, 0, ImVec2(0, 20.0f));

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && std::filesystem::is_directory(i.path().string())) {
                path = i.path().string();
                is_app_options = false;
            }
            else if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && !std::filesystem::is_directory(i.path().string())) {
                std::system(("xdg-open " + i.path().string()).c_str());
                is_app_options = false;
            }
            else if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
                is_app_options = true;
                is_dir_options = false;
                mouse_pos = ImGui::GetMousePos();
                clicked_path = i.path().string();
            }
            else if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(1)) {
                is_dir_options = true;
                is_app_options = false;
                mouse_pos = ImGui::GetMousePos();
                clicked_path = i.path().string();
            }
        }

        if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(1)) {
            is_dir_options = true;
            is_app_options = false;
            mouse_pos = ImGui::GetMousePos();
            clicked_path = path;
        }

        if (is_app_options) {
            if (width - mouse_pos.x < 200)
                mouse_pos.x -= 200;
            if (height - mouse_pos.y < 200)
                mouse_pos.y -= 200;

            if (ImGui::IsMouseClicked(0) && (ImGui::GetMousePos().x < mouse_pos.x || ImGui::GetMousePos().x > mouse_pos.x + 200 ||
                                             ImGui::GetMousePos().y < mouse_pos.y || ImGui::GetMousePos().y > mouse_pos.y + 200)) {
                is_app_options = false;
            }

            ImGui::SetNextWindowPos(mouse_pos);
            ImGui::SetNextWindowSize(ImVec2(200, 200));

            ImGui::Begin("**options", &is_app_options,
                         ImGuiWindowFlags_NoResize
                             | ImGuiWindowFlags_NoMove
                             | ImGuiWindowFlags_NoCollapse
                             | ImGuiWindowFlags_NoTitleBar
                         );

            ImGui::Selectable("Open", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                if (std::filesystem::is_directory(clicked_path))
                    path = clicked_path;
                else
                    std::system(("xdg-open " + clicked_path).c_str());
                is_app_options = false;
            }

            ImGui::Selectable("New folder", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                is_create_folder = true;
            }

            if (is_create_folder) {
                ImGui::InputText("##new name", new_name, 100);
                ImGui::SameLine();
                if (ImGui::Button("Ok") && strcmp(new_name, "") != 0) {
                    std::system(("mkdir " + clicked_path.substr(0, clicked_path.rfind('/')) + "/" + new_name).c_str());
                    is_app_options = false;
                    is_create_folder = false;
                    new_name[0] = '\0';
                }
            }

            ImGui::Selectable("New file", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                is_create_file = true;
            }

            if (is_create_file) {
                ImGui::InputText("##new name", new_name, 100);
                ImGui::SameLine();
                if (ImGui::Button("Ok") && strcmp(new_name, "") != 0) {
                    std::system(("touch " + clicked_path.substr(0, clicked_path.rfind('/')) + "/" + new_name).c_str());
                    is_app_options = false;
                    is_create_file = false;
                    new_name[0] = '\0';
                }
            }

            ImGui::Selectable("Cut", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                copy_path = clicked_path;
                is_app_options = false;
                is_cut = true;
            }

            ImGui::Selectable("Copy", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                copy_path = clicked_path;
                is_app_options = false;
                is_cut = false;
            }

            ImGui::BeginDisabled(copy_path.empty() ? true : false);
            ImGui::Selectable("Paste", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && copy_path != clicked_path) {
                if (std::filesystem::is_directory(copy_path)) {
                    std::system(("cp -r " + copy_path + " " + clicked_path).c_str());
                    if (is_cut)
                        std::system(("rm -rf " + copy_path).c_str());
                }
                else {
                    std::system(("cp " + copy_path + " " + clicked_path).c_str());
                    if (is_cut)
                        std::system(("rm " + copy_path).c_str());
                }

                copy_path = "";
                is_app_options = false;
                is_cut = false;
            }
            ImGui::EndDisabled();

            ImGui::Selectable("Rename", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                is_rename = true;
            }

            if (is_rename) {
                ImGui::InputText("##new name", new_name, 100);
                ImGui::SameLine();
                if (ImGui::Button("Ok") && strcmp(new_name, "") != 0) {
                    std::system(("mv " + clicked_path + " " + clicked_path.substr(0, clicked_path.rfind('/')) + "/" + new_name).c_str());
                    is_app_options = false;
                    is_rename = false;
                    new_name[0] = '\0';
                }
            }

            ImGui::Selectable("Compress", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
//                std::cout << ("tar czf " + clicked_path + ".tar.gz " + clicked_path).c_str() << '\n';
                std::system(("tar czf " + clicked_path + ".tar.gz " + clicked_path).c_str());

                is_app_options = false;
            }

            ImGui::Selectable("Remove", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                if (std::filesystem::is_directory(clicked_path))
                    std::system(("gio trash " + clicked_path).c_str());
                else
                    std::system(("gio trash " + clicked_path).c_str());

                    is_app_options = false;
            }

            ImGui::Selectable("Properties", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                if (std::filesystem::is_directory(clicked_path))

                is_app_options = false;
            }

            ImGui::End();
        }

        if (is_dir_options) {
            if (width - mouse_pos.x < 200)
                mouse_pos.x -= 200;
            if (height - mouse_pos.y < 200)
                mouse_pos.y -= 200;

            if (ImGui::IsMouseClicked(0) && (ImGui::GetMousePos().x < mouse_pos.x || ImGui::GetMousePos().x > mouse_pos.x + 200 ||
                                             ImGui::GetMousePos().y < mouse_pos.y || ImGui::GetMousePos().y > mouse_pos.y + 200)) {
                is_dir_options = false;
            }

            ImGui::SetNextWindowPos(mouse_pos);
            ImGui::SetNextWindowSize(ImVec2(200, 200));

            ImGui::Begin("**options", &is_app_options,
                         ImGuiWindowFlags_NoResize
                             | ImGuiWindowFlags_NoMove
                             | ImGuiWindowFlags_NoCollapse
                             | ImGuiWindowFlags_NoTitleBar
                         );

            ImGui::Selectable("New folder", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                is_create_folder = true;
            }

            if (is_create_folder) {
                ImGui::InputText("##new name", new_name, 100);
                ImGui::SameLine();
                if (ImGui::Button("Ok") && strcmp(new_name, "") != 0) {
                    std::system(("mkdir " + clicked_path + "/" + new_name).c_str());
                    is_dir_options = false;
                    is_create_folder = false;
                    new_name[0] = '\0';

                }
            }

            ImGui::Selectable("New file", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                is_create_file = true;
            }

            if (is_create_file) {
                ImGui::InputText("##new name", new_name, 100);
                ImGui::SameLine();
                if (ImGui::Button("Ok") && strcmp(new_name, "") != 0) {
                    std::system(("touch " + clicked_path + "/" + new_name).c_str());
                    is_dir_options = false;
                    is_create_file = false;
                    new_name[0] = '\0';
                }
            }

            ImGui::BeginDisabled(copy_path.empty() ? true : false);
            ImGui::Selectable("Paste", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && copy_path != clicked_path) {
                if (std::filesystem::is_directory(copy_path)) {
                    std::system(("cp -r " + copy_path + " " + clicked_path).c_str());
                if (is_cut)
                        std::system(("rm -rf " + copy_path).c_str());
                }
                else {
                    std::system(("cp " + copy_path + " " + clicked_path).c_str());
                    if (is_cut)
                        std::system(("rm " + copy_path).c_str());
                }

                copy_path = "";
                is_dir_options = false;
                is_cut = false;
            }
            ImGui::EndDisabled();

            ImGui::Selectable("Properties", false, 0, ImVec2(0, 20));
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                if (std::filesystem::is_directory(clicked_path))

                is_dir_options = false;
            }

            ImGui::End();
        }

        if (is_menu_options) {
            if (width - mouse_pos.x < 210)
                mouse_pos.x -= 200;
            if (height - mouse_pos.y < 210)
                mouse_pos.y -= 200;
            ImGui::SetNextWindowPos(mouse_pos);
            ImGui::SetNextWindowSize(ImVec2(200, 200));

            ImGui::Begin("**menuoptions", &is_app_options,
                         ImGuiWindowFlags_NoResize
                             | ImGuiWindowFlags_NoMove
                             | ImGuiWindowFlags_NoCollapse
                             | ImGuiWindowFlags_NoTitleBar
                         );

            ImGui::Checkbox("Show hidden", &show_hidden);


            ImGui::End();
        }

        ImGui::EndChild();
    }
}

void left_window(int width, int height, std::string &path, std::string *bookmarks) {
    std::string intermediate = "";
    ImGui::SameLine();
    ImGui::SetCursorPosX(0);
    if (ImGui::BeginChild("Bookmarks", ImVec2(200, 0), true,
                          ImGuiWindowFlags_NoResize
                              | ImGuiWindowFlags_NoCollapse
                              | ImGuiWindowFlags_NoMove
                              | ImGuiWindowFlags_NoTitleBar)) {

        for (int i = 0; i < 6; i++) {
            if (bookmarks[i] != getenv(HOMEDIR)) {
                intermediate = bookmarks[i].substr(bookmarks[i].rfind('/')).erase(0, 1);
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

void top_window() {

}



int main() {
    std::string path = (std::string) getenv(HOMEDIR);
    char search_path[100] = "";
    char new_name[100] = "";
    std::string clicked_path = "";
    std::string copy_path = "";

    int width = 820;
    int height = 480;
    int icon_width = 64, icon_height = 64;
    bool is_app_options = false;
    bool is_menu_options = false;
    bool is_dir_options = false;
    bool show_hidden = false;
    bool is_cut = false;
    bool is_rename = false;
    bool is_create_folder = false;
    bool is_create_file = false;
    ImVec2 mouse_pos;

    std::string bookmarks[6];
    bookmarks[0] = path;
    bookmarks[1] = path + "/Documents";
    bookmarks[2] = path + "/Downloads";
    bookmarks[3] = path + "/Music";
    bookmarks[4] = path + "/Pictures";
    bookmarks[5] = path + "/Videos";

    assert(glfwInit() && "Could not init GLFW!");

    GLFWwindow *window = glfwCreateWindow(width, height, "VSFM", nullptr, nullptr);
	
    glfwSetErrorCallback(error_callback);
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

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1, 0.1, 0.1, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //ImGui::ShowDemoWindow(&show_demo_window);
	
        // main window
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2{(float) width, (float) height});
        ImGui::Begin("MainWindow", 0,
                     ImGuiWindowFlags_NoResize
                     | ImGuiWindowFlags_NoMove
                     | ImGuiWindowFlags_NoCollapse
                     | ImGuiWindowFlags_NoBringToFrontOnFocus
                     | ImGuiWindowFlags_NoTitleBar);
        {
            left_window(width, height, path, bookmarks);
            main_window(width, height, path, search_path, is_app_options, is_menu_options, mouse_pos, show_hidden, clicked_path,
                        copy_path, is_cut, new_name, is_rename, is_create_folder, is_create_file, is_dir_options);
        }
        
        glfwGetWindowSize(window, &width, &height);

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    return 0;
}
