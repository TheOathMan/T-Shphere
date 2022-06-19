#pragma once

//struct BitmapData{
//    unsigned char* pixels;
//    int width,hight;
//    GLuint gpu_id;
//    BitmapData() : pixels(nullptr), width(0), hight(0){}
//    BitmapData(unsigned char* pixels ,int width ,int hight);
//    void To_RBGA(unsigned char* inData);
//};

struct BitmapData{
    unsigned int gpu_id   =  0;
    int width             = -1;
    int hight             = -1;
    unsigned char* pixels = nullptr;

    BitmapData() = default;
    //uploads data to gpu
    BitmapData(unsigned char* pixels ,int width ,int hight);
    BitmapData(int gpu_id ,int width ,int hight);
    
    BitmapData& operator = (const BitmapData& rhs) = delete;
    BitmapData(const BitmapData& rhs) = delete;

    BitmapData& operator = (BitmapData&& rhs) noexcept;
    BitmapData(BitmapData&& rhs);

    void To_RBGA(unsigned char* inData);
    ~BitmapData();
};

void MakeLogoBitmaps(BitmapData& bitmapdata, const unsigned char* font_data,int glyph, float scale,int EdgeOffsetX = 6,int EdgeOffsetY = 6,int offx=0,int offY=0);
void UplaodImageToGpu(BitmapData* bitmap);
void GetLogoBitmapsSize(BitmapData& bitmap, const unsigned char* font_data,int glyph, float scale,int* width,int* height);
