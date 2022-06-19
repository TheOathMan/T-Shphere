
#pragma once
#include <string>
#include "BitmapData.h"
#include "config.h"
#include <unordered_map>

enum FileInfoState{
    FileInfoState_Fine         = 1 << 0,
    FileInfoState_Failed       = 1 << 1,
    FileInfoState_Icon_OK      = 1 << 2,
    FileInfoState_Icon_Failed  = 1 << 3
};

class FileInfo{
    public:
    int file_info_state=0;
    bool Selection = false;
    size_t win_size; // file windows size
    STD_STR FilePath;

    FileInfo() = default;
    FileInfo(FileInfo&&) noexcept = default;

    FileInfo& operator = (const FileInfo& rhs) = delete;
    FileInfo(const FileInfo& rhs) = delete;

    FileInfo(const M_CHAR* file_path, bool processIcon = true);
    //~FileInfo();

    inline const bool is_good()                    const { return file_info_state & FileInfoState_Fine;};
    inline const std::string& display_name()           const { return disply_name;};  
    // File Or Directory
    inline const std::string& get_type()           const { return type;};
    // The System File type
    inline const std::string& get_name_type()      const { return type_name;};
    inline const std::string& get_size()           const { return size;};
    inline const std::string& get_device()         const { return device;};
    inline const std::string& get_created_time()   const { return created_time;};
    inline const std::string& get_modified_time()  const { return modified_time;};
    inline const BitmapData*  get_file_icon()      const { return file_icon;};
    void  clear_cashed_icons() { icons_map.clear();};

    int DeletetoRecycle();
    private:
    static std::unordered_map <int,BitmapData> icons_map;
    std::string disply_name,type,size,device,created_time,modified_time,type_name;
    BitmapData* file_icon;
};

