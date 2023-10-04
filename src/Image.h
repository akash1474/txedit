#pragma once

#include "imgui.h"
#include "spdlog/fmt/bundled/format.h"
#include <gl/gl.h>
#include <string>


#define GL_CLAMP_TO_EDGE 0x812F
#define GL_CLAMP 0x2900
#define GL_CLAMP_TO_BORDER 0x812D


namespace OpenGL {

	class Image
	{
	public:
		Image(std::string_view path);
		Image(int width, int height, const void* data = nullptr);
		Image(){}
		~Image();

		bool LoadFromBuffer(const unsigned char* data,int size);
		GLuint GetTexture() const { return mTexture; }


		int GetWidth() const { return mWidth; }
		int GetHeight() const { return mHeight; }

	private:
		int mWidth = 0, mHeight = 0;
		std::string mFilePath;
	    GLuint mTexture;
	};

}



