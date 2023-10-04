#include "pch.h"
#include "Image.h"
#include "imgui.h"
#include "stb_img.h"

OpenGL::Image::Image(std::string_view file_path):mFilePath{file_path}{}
OpenGL::Image::Image(int width, int height, const void* data):mWidth{width},mHeight{height}{};


OpenGL::Image::~Image(){}

bool OpenGL::Image::LoadFromBuffer(const unsigned char* data,int size){
	unsigned char* img_data = stbi_load_from_memory(data, size, &mWidth, &mHeight, 0, 4); // rgba channels
	if(img_data==NULL){
		GL_CRITICAL("Failed to load image");
		return false;
	}

    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data);

    // Freeing the resources
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(img_data);

    return true;
}
