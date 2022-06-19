#include <iostream>
#include "BitmapData.h"
#include "../outsrc/GLFW/glfw3.h"
#include <cstdlib>
#include "..\outsrc\imgui\imstb_truetype.h"


void UplaodImageToGpu(BitmapData* bitmap){
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &bitmap->gpu_id);
    glBindTexture(GL_TEXTURE_2D, bitmap->gpu_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap->width, bitmap->hight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap->pixels);
    glBindTexture(GL_TEXTURE_2D, last_texture); 
}

BitmapData::BitmapData(unsigned char* pixels ,int width ,int hight) : width(width),hight(hight),gpu_id(0) {
    if(pixels) To_RBGA(pixels);
    UplaodImageToGpu(this);
}
BitmapData::BitmapData(int gpu_id ,int width ,int hight): gpu_id(gpu_id),width(width),hight(hight){}

BitmapData& BitmapData::operator = (BitmapData&& rhs) noexcept 
{
    pixels     = rhs.pixels;
    width      = rhs.width;
    hight      = rhs.hight; 
    gpu_id     = rhs.gpu_id; 

    rhs.pixels = nullptr; 
    rhs.width  = -1;
    rhs.hight  = -1;
    rhs.gpu_id = 0;
    return *this; 
 };
BitmapData::BitmapData(BitmapData&& rhs) : 
    pixels(rhs.pixels),
    width(rhs.width),
    hight(rhs.hight),
    gpu_id(rhs.gpu_id) 
{
    rhs.pixels = nullptr;
    rhs.width  = -1;
    rhs.hight  = -1;
    rhs.gpu_id = 0;
};

BitmapData::~BitmapData(){
    if(pixels)free(pixels);
    //if(gpu_id>0)glDeleteTextures(1,&gpu_id); 
    
}


void BitmapData::To_RBGA(unsigned char* inData)
{
    this->pixels = new unsigned char[width * hight * 4];
    for (size_t i = 0; i < width * hight; i++)
    {
        unsigned char* inp = inData + i;
        unsigned char* newdataP = this->pixels + (i * 4);
        newdataP[0] = 1 - inp[0];
        newdataP[1] = 1 - inp[0];
        newdataP[2] = 1 - inp[0];
        newdataP[3] = inp[0];
    }
}


void MakeLogoBitmaps(BitmapData& bitmap, const unsigned char* font_data,int glyph, float scale,int EdgeOffsetX,int EdgeOffsetY,int offx,int offY){
    static stbtt_fontinfo font;
    static int fres = stbtt_InitFont(&font,font_data,0);
    stbtt_vertex* vertices = NULL;
    int versize = stbtt_GetGlyphShape(&font, glyph, &vertices);
    float _scale = stbtt_ScaleForPixelHeight(&font, scale);
    int ix0, iy0, ix1, iy1;
    stbtt__bitmap gbm;
    stbtt_GetGlyphBitmapBoxSubpixel(&font, glyph, _scale, _scale, 0.0f, 0.0f, &ix0, &iy0, &ix1, &iy1);
    gbm.w = (ix1 - ix0) + EdgeOffsetX;
    gbm.h = (iy1 - iy0) + EdgeOffsetY;
    gbm.pixels = NULL; // in case we error
    if (gbm.w && gbm.h) {
        gbm.pixels = new unsigned char[gbm.w * gbm.h];
        if (gbm.pixels) {
            gbm.stride = gbm.w;

            stbtt_Rasterize(&gbm, 0.35f, vertices, versize, _scale, _scale, EdgeOffsetX / 2.0f, EdgeOffsetY / 2.0f, ix0+offx, iy0+offY, 1, NULL);
        }
    }
    bitmap = std::move(BitmapData(gbm.pixels,gbm.w ,gbm.h));
    stbtt_FreeBitmap(gbm.pixels,NULL);
}


void GetLogoBitmapsSize(BitmapData& bitmap, const unsigned char* font_data,int glyph, float scale,int* width,int* height){
    static stbtt_fontinfo font;
    static int fres = stbtt_InitFont(&font,font_data,0);
    float _scale = stbtt_ScaleForPixelHeight(&font, scale);
    int ix0, iy0, ix1, iy1;
    stbtt_GetGlyphBitmapBoxSubpixel(&font, glyph, _scale, _scale, 0.0f, 0.0f, &ix0, &iy0, &ix1, &iy1);
    *width  = (ix1 - ix0);
    *height = (iy1 - iy0);
}