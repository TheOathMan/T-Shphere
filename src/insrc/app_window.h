#pragma once
//#define IMGUI_USE_WCHAR32
#define IMGUI_DEFINE_MATH_OPERATORS
#include <vector>
#include <string>
#include "../outsrc/imgui/imgui_impl_glfw.h"
#include "../outsrc/imgui/imgui_impl_opengl2.h"
#include "../outsrc/imgui/imgui.h"
#include "../outsrc/imgui/imgui_internal.h"
#include "../outsrc/GLFW/glfw3.h"

//struct ScreenSize{ int width=0,hight=0; }; 


class App_Window {
public:
	App_Window(const char* window_name,int width, int height, GLFWwindow* srd_window = NULL);
	virtual ~App_Window();

	static std::vector<App_Window*> wins32;
	virtual void OnWindStart();
	virtual void OnUpdate();
	virtual void OnInput();

	void SetBackgroundColor(float red, float green, float blue, float alpha);

	ImVec2i get_window_res();	      
	ImVec2i get_screenSize();	      
	ImVec2i get_win_pos();	      
	std::vector<std::string>* Get_dropped_files();	      
	inline GLFWwindow* get_window()		  const { return window;}
	inline const char* get_window_name()  const { return windowName;}
	inline bool IsWindowOpen()					{ return !glfwWindowShouldClose(window); }
	inline bool IsWindowVisible()               { int a; glfwGetWindowSize(window,&a,&a); return a;}
private:
	const char* windowName;
	float background_col[4];
	ImVec2i screenSize;

protected:
	GLFWwindow* window = nullptr;
};