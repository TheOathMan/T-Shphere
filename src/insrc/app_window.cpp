#include "app_window.h" 
#include <stdio.h>
#include <unordered_map>
#include <string>
#include <iostream>
//thread_local ImGuiContext* MyImGuiTLS; // thread local context if need. ned to be activated from imconfig.h 


namespace AppMain{
    void AppAwake(App_Window* window);
    void AppStart(App_Window* window);
    void AppUpdate(App_Window* window, float deltaTime);
    void AppClosed(App_Window* window);
}

struct floatingGuiWin{
    bool is_open = true;
    bool grapped=false;
    ImVec2 grappedPos = ImVec2(0.0f,0.0f);
};

// glfw window
struct MainWinData{
    ImVec2i WinPos = ImVec2i(-1,-1);
    ImVec2i WinRes = ImVec2i(-1,-1);
};

//incase we have multiple windows
std::unordered_map<GLFWwindow*,MainWinData> MainWinMap;   
std::unordered_map<GLFWwindow*,floatingGuiWin> FloatingGuiWinsMap;   
std::vector<App_Window*> App_Window::wins32;
std::vector<std::string> Dropped_Files;

ImVec2i App_Window::get_window_res(){
    return ImVec2i(MainWinMap.at(window).WinRes.x, MainWinMap.at(window).WinRes.y);
}

ImVec2i App_Window::get_win_pos(){
    return ImVec2i(MainWinMap.at(window).WinPos.x, MainWinMap.at(window).WinPos.y);
}
std::vector<std::string>* App_Window::Get_dropped_files(){
    if(Dropped_Files.size())
        return &Dropped_Files;
    return nullptr;
}	      

App_Window::App_Window(const char* window_name, int width, int height, GLFWwindow* srd_window): windowName(window_name)
{
    AppMain::AppAwake(this);

    window = glfwCreateWindow(width, height, windowName, NULL, srd_window);
    MainWinMap[window].WinRes = ImVec2i(width,height);

    if (!window) {
		glfwTerminate();
        perror ("Window failed at initialization");
		exit(EXIT_FAILURE);
	}

    glfwMakeContextCurrent(window);
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    screenSize.x = mode->width, screenSize.y = mode->height;
    FloatingGuiWinsMap.insert({window,floatingGuiWin()});
}

void App_Window::OnWindStart()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();
    AppMain::AppStart(this);

    glfwSetWindowPosCallback(window, [](GLFWwindow* w,int x,int y )  { MainWinMap[w].WinPos = ImVec2i(x,y); });
    glfwSetWindowSizeCallback(window, [](GLFWwindow* w,int x,int y ) { MainWinMap[w].WinRes = ImVec2i(x,y); });
    glfwSetWindowPos(window,500,200); //! center pos
    glfwSetDropCallback(window,[](GLFWwindow*,int s,const char* c[]) { Dropped_Files.clear(); for(int i =0; i < s; ++i){Dropped_Files.push_back(c[i]);}});
}


void App_Window::OnUpdate()
{
    static ImVec2 wins;
    static int focused = 0;
    
    bool Window_active = focused && get_window_res().x > 50 && get_window_res().y > 50;

    if(Window_active && ImGui::GetCurrentContext()->FrameCount > 10) //! needs to ne based on window size delta
        glfwSetWindowSize(window, wins.x,wins.y);

    glfwMakeContextCurrent(window);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(.0f,.0f,.0f,.0f);

    //frame start.
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    auto&& fgwm_access  = FloatingGuiWinsMap.at(window); 
    bool& is_open       = fgwm_access.is_open;
    bool& is_grapped    = fgwm_access.grapped;
    ImVec2& grapped_pos = fgwm_access.grappedPos;

    ImGuiContext* contex = ImGui::GetCurrentContext();

    ImVec2 mosuePos = ImGui::GetMousePos();
    ImVec2i windowPosSS = get_win_pos(); // defuat is win pos
    ImVec2 MousePosGlobal(windowPosSS.x + mosuePos.x, windowPosSS.y + mosuePos.y);
        
    // prevent updating ImGuiNextWindowDataFlags_HasSize flag
    if(Window_active)
        ImGui::SetNextWindowSize(get_window_res().to_fvec()); 
    ImGui::SetNextWindowPos(ImVec2(0.0f,0.0f)); 
    
    ImGui::Begin(windowName,&is_open,ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    //ImGui::Begin(windowName,&is_open, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    //glfwSetWindowSize(window, ImGui::GetWindowSize().x,ImGui::GetWindowSize().y);
    wins = ImGui::GetWindowSize();
    //std::cout << wins.x << "  " << wins.y << std::endl;
    ImGuiWindow* gui_win = ImGui::GetCurrentWindow();//gui_win->
    bool isMoseHoveringOverTitleBar = mosuePos.y < gui_win->TitleBarRect().Max.y && (contex->ActiveId != gui_win->GetID("#CLOSE")); 
    
    
    // grap window from top bar
    if(ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow) && focused){
        if(isMoseHoveringOverTitleBar || is_grapped){
            glfwSetWindowPos(window, MousePosGlobal.x - grapped_pos.x, MousePosGlobal.y - grapped_pos.y);
            is_grapped = true;
        }
    }
    else
    {
        is_grapped  = false;
        grapped_pos = ImGui::GetMousePos();
    }

    AppMain::AppUpdate(this,ImGui::GetIO().DeltaTime);

    
    ImGui::End();


    //frame end.
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    if(!is_open)
        glfwSetWindowShouldClose(window,true);

    focused = glfwGetWindowAttrib(window, GLFW_FOCUSED);
}

void App_Window::OnInput()
{

}

App_Window:: ~App_Window() 
{
    glfwDestroyWindow(window);
    AppMain::AppClosed(this);
}
void App_Window::SetBackgroundColor(float red, float green, float blue, float alpha)
{
    background_col[0] = red;
    background_col[1] = green;
    background_col[2] = blue;
    background_col[3] = alpha;
}

// draw textured quad opengl
void RawQuadTexture(int id)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, id);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f, 1.0f);

    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-1.0f, -1.0f);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1.0f, -1.0f);

    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(1.0f, 1.0f);
    glEnd();
}
