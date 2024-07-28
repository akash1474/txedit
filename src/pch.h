#pragma once
#define GL_BUILD_OPENGL2
#define APP_NAME "TxEdit"




#include <iostream>
#include <cmath>

#include <future>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm>

#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#ifdef GL_BUILD_OPENGL2
	#include "imgui_impl_opengl2.h"
#else
	#include "imgui_impl_opengl3.h"
#endif
#include "imgui_internal.h"



#include "FontAwesome6.h"
#include "Log.h"
#include "utils.h"
#include "Timer.h"

//Icon and Font
#include "fa-solid-900.h"
#include "fa-regular.h"
#include "recursive_linear_medium.h"
#include "monolisa_medium.h"
#include "images.h"

