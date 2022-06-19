#include "config.h"
#include <Windows.h>
#include <stdio.h>


int MulticharToWchar(std::vector<wchar_t>& wc_buf,const char* str){
    const int str_wsize = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if(wc_buf.size() < str_wsize) wc_buf.resize(str_wsize);
    int res = MultiByteToWideChar(CP_UTF8, 0, str, -1, (wchar_t*)&wc_buf[0], str_wsize);
    if (!res) wc_buf.resize(0);
    return res;
}
int MulticharToWchar(wchar_t* wc_buf,const char* str){
    const int str_wsize = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    int res = MultiByteToWideChar(CP_UTF8, 0, str, -1, wc_buf, str_wsize);
    if(!res) wc_buf = nullptr;
    return res;
}
int WcharToMultichar(std::vector<char>& c_buf,const wchar_t* wstr){
    int size = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    if(c_buf.size() < size) c_buf.resize(size);
    int res = WideCharToMultiByte(CP_ACP, 0, wstr, -1, &c_buf[0], size, NULL, NULL);
    if(!res) c_buf.resize(0);
    return res;
}
int WcharToMultichar(char* c_buf,const wchar_t* wstr){
    int size = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    int res = WideCharToMultiByte(CP_ACP, 0, wstr, -1, c_buf, size, NULL, NULL);
    if(!res) c_buf=nullptr;
    return res;
}

const char* ToLastSlash(const char* str){const char*p; for(p = str + strlen(str); *(p-1) != '\\' && *(p-1) != '/' && p != str; --p ){} return p;} 

template<typename T,typename T2> void vc_srecpy(T& v,T2 str){int s = strlen(str)+1; v.resize(s); strcpy(&v[0],str);}
template<typename T,typename T2> void wvc_srecpy(T& v,T2 str){int s = wcslen(str)+1; v.resize(s); wcscpy(&v[0],str);}

//== char to wide char str copy ==
void Dynamic_strcpy(std::vector<wchar_t>& dest, const char* src ){MulticharToWchar(dest,src);}
void Dynamic_strcpy(std::vector<wchar_t>& dest,char* src ){MulticharToWchar(dest,src);}
void Dynamic_strcpy(wchar_t* dest,const char* src ){MulticharToWchar(dest,src);}
//== wide char to char str copy==
void Dynamic_strcpy(std::vector<char>& dest,wchar_t* src ){WcharToMultichar(dest,src);}
void Dynamic_strcpy(std::vector<char>& dest, const wchar_t* src ){WcharToMultichar(dest,src);}
void Dynamic_strcpy(char* dest, const wchar_t* src ){WcharToMultichar(dest,src);}
//== wchar str copy ==
void Dynamic_strcpy(wchar_t*& dest,const  wchar_t* src ){wcscpy(dest,src);}
void Dynamic_strcpy(wchar_t*& dest, wchar_t* src ){wcscpy(dest,src);}
void Dynamic_strcpy(std::vector<wchar_t>& dest,const wchar_t* src ){wvc_srecpy(dest,src);}
void Dynamic_strcpy(std::vector<wchar_t>& dest,wchar_t* src ){wvc_srecpy(dest,src);}
//== char str copy ==
void Dynamic_strcpy(char*& dest,const char* src ){strcpy(dest,src);}
void Dynamic_strcpy(char*& dest, char* src ){strcpy(dest,src);}
void Dynamic_strcpy(std::vector<char>& dest,const char* src ){vc_srecpy(dest,src);}
void Dynamic_strcpy(std::vector<char>& dest,char* src ){vc_srecpy(dest,src);}

int d_strlen(const char* c ){ return strlen(c); }
int d_strlen(const wchar_t* c ){ return wcslen(c); }

