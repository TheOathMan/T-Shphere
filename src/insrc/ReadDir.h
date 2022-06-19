#pragma once
#include <vector>
#include <string>
#include "FileInfo.h"

#include "config.h"

enum class  filetype : short{file,folder,link,other};
struct DirCont{STD_STR Name;filetype type;};

struct ReadDir{
    STD_STR PathToDir;
    std::vector<DirCont> result;
    ReadDir() = default;
    ReadDir(const STD_STR& dirname);
    ReadDir(STD_STR&& dirname);
    const std::vector<std::string>& GetFiles();
    const std::vector<std::string>& GetFolders();
    std::vector<FileInfo>& GetFileInfos();
    inline const bool is_good(){return m_is_good;}

    bool find(std::string name);

    //void ResetSelection();
    //std::string selection;  //* this is an array of bool, but we used string to take advantage of small string optimization.
    private: 
    std::vector<std::string> _FilesPath;
    std::vector<std::string> _FoldersPath;
    std::vector<FileInfo>  _file_infos;
    bool m_is_good=true;
    void init();
};