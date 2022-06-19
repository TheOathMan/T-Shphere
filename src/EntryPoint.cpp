#include "insrc/app_window.h"
#include <stdio.h>

int main(int, char**)
{
    glfwSetErrorCallback([](int error, const char* description) {fprintf(stderr, "Glfw Error %d: %s\n", error, description); });
    if (!glfwInit())
        return 1;

    App_Window* main = new App_Window("T-Shphere", 700, 650);
    App_Window::wins32.push_back(main);
    glfwSwapInterval(1);

    // windows start
    for (auto& WINS : App_Window::wins32) {
        WINS->OnWindStart();
    }

    while (main->IsWindowOpen())
    {
        //windows update
        for (auto& WINS : App_Window::wins32) {
            WINS->OnUpdate();
            glfwSwapBuffers(WINS->get_window());
        }        
        glfwPollEvents();
    }

    //destruction
    for (auto& WINS : App_Window::wins32) {
        delete WINS;
    }

    glfwTerminate();

    return 0;
}
