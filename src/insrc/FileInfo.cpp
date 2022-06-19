#include <iostream>
#include <sys/stat.h>
#include "FileInfo.h"
#include <Windows.h>
#include <vector>

// all new icons will be hashed once, nothing will be freed from the gpu at runtime.
// cpu side icon pixels will be freed once we don't need them anymore.
std::unordered_map <int,BitmapData> FileInfo::icons_map;


#if defined USE_WIDE_CHARS
    #define FUNC_STAT   _wstat64
    #define STRC_STAT   _stat64 
    #define TIME_T      __time64_t
    #define LOCALTIME   _localtime64

    #define FILEINFO SHFILEINFOW
    #define GETFILEINFO SHGetFileInfoW
#else
    #define FUNC_STAT   stat
    #define STRC_STAT   stat 
    #define TIME_T      time_t
    #define LOCALTIME   localtime

    #define FILEINFO SHFILEINFO
    #define GETFILEINFO SHGetFileInfo
#endif

struct STRC_STAT fileInfo;
struct tm * timeinfo;
std::string timeformate(TIME_T& t){
    timeinfo = LOCALTIME (&t);
    char buffer[MAX_FILE_NAME];
    strftime(buffer, sizeof(buffer), "%d/%m/%y  %I:%M %p", timeinfo);
    return std::string{buffer};
}

int GetBitmapPixels(HBITMAP& hBitmap,BitmapData& p)
{
	BITMAP bmp;
	DWORD dwLen;
	int x, y;

	GetObject(hBitmap, sizeof(bmp), &bmp);
	if (bmp.bmBitsPixel != 32)
		return 0;

	dwLen = bmp.bmWidth * bmp.bmHeight * (bmp.bmBitsPixel / 8);
    BYTE* m_p = (BYTE *)malloc(dwLen);	
	memset(m_p, 0, dwLen);

	GetBitmapBits(hBitmap, dwLen, m_p);

    for (y = 0; y < bmp.bmHeight; ++y) {
    	BYTE *px = m_p + bmp.bmWidth * 4 * y;
    
    	for (x = 0; x < bmp.bmWidth; ++x) {
            BYTE a1 = px[0],a2 = px[1],a3 = px[2];
            px[0] = a3;
            px[1] = a2;
            px[2] = a1;
    		px += 4;
    	}
    }
    p.pixels = m_p; p.width = bmp.bmWidth; p.hight = bmp.bmHeight;
    UplaodImageToGpu(&p);
    return 1;
}



FileInfo::FileInfo(const M_CHAR* file_path,bool processIcon) : FilePath(file_path){
    if (FUNC_STAT(file_path, &fileInfo) != 0) {  // Use stat() to get the info
        std::wcerr << L"[" << file_path << L"] " << strerror(errno) << '\n';
        file_info_state |= FileInfoState_Failed;
        return;
    }

    if ((fileInfo.st_mode & S_IFMT) == S_IFDIR) { // From sys/types.h
        type = "Directory";
    } else {
        type = "File";
    } 

    size = type[0] ==  'F' ? std::to_string(fileInfo.st_size) : "";               // Size in bytes
    win_size = fileInfo.st_size;
    created_time = timeformate(fileInfo.st_ctime);
    modified_time = timeformate(fileInfo.st_mtime);
    device += (char)(fileInfo.st_dev + 'A');
    file_info_state |= FileInfoState_Fine;

    // Get File Icons ===================== 
    //
    FILEINFO info;    
    GETFILEINFO(file_path, FILE_ATTRIBUTE_NORMAL, &info, sizeof(info), SHGFI_TYPENAME | SHGFI_DISPLAYNAME | SHGFI_ICON | SHGFI_SMALLICON);
    ICONINFO stIconInfo;    
    GetIconInfo(info.hIcon, &stIconInfo);

#if defined USE_WIDE_CHARS
    std::vector<char> c_buffer;
    Dynamic_strcpy(c_buffer, info.szDisplayName);
    disply_name =   &c_buffer[0];
    Dynamic_strcpy(c_buffer, info.szTypeName);
    type_name =   &c_buffer[0];
#else
    disply_name = info.szDisplayName;
    type_name =   info.szTypeName;
#endif
    if(processIcon){
        if(icons_map.find(info.iIcon) == icons_map.end()){
            HBITMAP hBmp = stIconInfo.hbmColor;
            //! hash duplicated icons using info.iIcon---------------
            BitmapData tfi;
            file_info_state |= GetBitmapPixels(hBmp,tfi) ? FileInfoState_Icon_OK : FileInfoState_Icon_Failed;
            icons_map[info.iIcon] = std::move (tfi); 
            file_icon = &icons_map.at(info.iIcon);
            //DEBUG_LOG("created");
        }
        else
        {
            //DEBUG_LOG("found");
            file_info_state |=FileInfoState_Icon_OK;
            auto && iconat = icons_map.at(info.iIcon);
            file_icon = &iconat;
        }
    }
    DestroyIcon(info.hIcon);
}

int FileInfo::DeletetoRecycle()
{
    auto fpnt =FilePath; fpnt += ((wchar_t)'\0');
	SHFILEOPSTRUCTW FileOp;
	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_DELETE;
	FileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR | FOF_ALLOWUNDO;
	FileOp.pFrom = fpnt.data();
	FileOp.pTo = NULL;
	return SHFileOperationW(&FileOp);
}
