#pragma once
#include <vector>
#include <iostream>

#define USE_WIDE_CHARS 

#if defined USE_WIDE_CHARS
    #define M_strcpy wcscpy
    #define M_strlen wcslen
    #define M_strcmp wcscmp
    #define FOLDER_DIALOG tinyfd_selectFolderDialogW(L"Open Folder",NULL)
    #define M_CHAR wchar_t
    #define STD_STR std::wstring
    #define IFSTREAM std::wifstream
    #define OFSTREAM std::wofstream
#else
    #define M_strcpy strcpy 
    #define M_strlen strlen 
    #define M_strcmp strcmp
    #define FOLDER_DIALOG tinyfd_selectFolderDialog("Open Folder",NULL)
    #define M_CHAR char
    #define STD_STR std::string
    #define IFSTREAM std::ifstream
    #define OFSTREAM std::ofstream
#endif

#define RESV_B_STRD(V,STR) if(M_strlen(STR)+1 > V.capacity()) V.resize(M_strlen(STR)+6)
#define RESV_B_STRW(V,STR) if(wcslen(STR)+1 > V.capacity()) V.resize(wcslen(STR)+6)
#define RESV_B_STRA(V,STR) if(strlen (STR)+1 > V.capacity()) V.resize(strlen(STR)+6)
#define MAX_FILE_NAME 260
#define MAX_PATH 1024
#define MAX_Indexes 64
#define VREF(vec) (&vec[0])
#define Byte_To_MB 0.000001f 
#define MB_to_Byte 1048576 

//debug
//#define TO_FILE_PTR (x) for(p = op + strlen(op); *p != '\\' && *p != '/' && p != op; --p ){}
const char* ToLastSlash(const char* str);
    
#define DEBUG_INFO std::cout << ToLastSlash(__FILE__) << " " << __LINE__ << ": ";
#define DEBUG_LOG(x)  do  {DEBUG_INFO; std::cout  << x << std::endl;}while(false)
#define DEBUG_LOGW(x) do  {DEBUG_INFO; std::wcout << x << std::endl;}while(false)


int MulticharToWchar(std::vector<wchar_t>& dest,const char* src);
int MulticharToWchar(wchar_t* wc_buf,const char* str);

int WcharToMultichar(std::vector<char>& dest,const wchar_t* src);
int WcharToMultichar(char* c_buf,const wchar_t* wstr);

//== char to wide char str copy ==
void Dynamic_strcpy(std::vector<wchar_t>& dest,char* src );
void Dynamic_strcpy(std::vector<wchar_t>& dest, const char* src );
void Dynamic_strcpy(wchar_t* dest,const char* src );
//== wide char to char str copy==
void Dynamic_strcpy(std::vector<char>& dest,wchar_t* src );
void Dynamic_strcpy(std::vector<char>& dest, const wchar_t* src );
void Dynamic_strcpy(char* dest, const wchar_t* src );
//== wchar str copy ==
void Dynamic_strcpy(wchar_t*& dest,const  wchar_t* src );
void Dynamic_strcpy(wchar_t*& dest, wchar_t* src );
void Dynamic_strcpy(std::vector<wchar_t>& dest,const wchar_t* src );
void Dynamic_strcpy(std::vector<wchar_t>& dest,wchar_t* src );
//== char str copy ==
void Dynamic_strcpy(char*& dest,const char* src );
void Dynamic_strcpy(char*& dest, char* src );
void Dynamic_strcpy(std::vector<char>& dest,const char* src );
void Dynamic_strcpy(std::vector<char>& dest,char* src );



int d_strlen(const char* c );
int d_strlen(const wchar_t* c );
float Repeat(float t, float length);

// cut file name from a path
template<class char_type>
int CutStrToPath(char_type* path){
    char_type* p;
    char_type* op = path;
    for(p = op + d_strlen(op); *p != (char_type)'\\' && *p != (char_type)'/' && p != op; --p ){} 
    if(p != op) {*p = (char_type)'\0'; return 1 ;};
    return 0; 
}


// cut extention name from a str
template<class char_type>
int CutStrToDot(char_type* path){
    char_type* p;
    char_type* op = path;
    for(p = op + d_strlen(op); *p != (char_type)'.' && p != op; --p ){} 
    if(p != op) {*p = (char_type)'\0'; return 1 ;};
    return 0; 
}

//return file name from a full path
template<class char_type>
char_type* CutStrToFile(char_type* path){
    char_type* p;
    char_type* op = path;
    for(p = op + d_strlen(op); *p != (char_type)'\\' && *p != (char_type)'/' && p != op; --p ){} 
    //if(p != op) *p = '\0'; 
    return p == path ? p : p+1;
}