
#include "ReadDir.h"
#include "../outsrc/dirent/dirent.h"

#if defined USE_WIDE_CHARS
    #define M_DIR       WDIR
    #define OPENDIR     _wopendir
    #define M_fprintf   fwprintf
    #define M_DIRBET    _wdirent
    #define M_READDIR   _wreaddir
    #define M_CLOSEDIR  _wclosedir
#else
    #define M_DIR       DIR
    #define OPENDIR     opendir
    #define M_fprintf   fprintf
    #define M_DIRBET    dirent
    #define M_READDIR   readdir
    #define M_CLOSEDIR  closedir
#endif


void ReadDir::init(){

    if(!PathToDir.empty() && PathToDir.back() == (M_CHAR)':') // to device path
        PathToDir += (M_CHAR)'\\';
    
	M_DIR *dir = OPENDIR(PathToDir.c_str());

	if (!dir) {
		M_fprintf(stderr,(const M_CHAR*)"Cannot open %s (%s)\n", PathToDir.c_str(), strerror(errno));
        m_is_good = false;
        return;
	}
	M_DIRBET *ent;
	while ((ent = M_READDIR(dir)) != NULL) {        
        STD_STR temps = ent->d_name;
        if( temps == L".." || temps == L".") continue;
		switch (ent->d_type) {
		case DT_REG:
             result.push_back({temps ,filetype::file});
			break;
		case DT_DIR:
            result.push_back({temps,filetype::folder});
			break;

		case DT_LNK:
            result.push_back({temps,filetype::link});
			break;

		default:
            result.push_back({temps,filetype::other});
		}
	}
    for(auto&& i : result){
        auto path = PathToDir;  
        if(path.back() != (M_CHAR)'\\') 
            path += (M_CHAR)'\\';
        path.append(i.Name);
        FileInfo file_info(path.c_str(), true);
        if (file_info.is_good()) // maybe optimize by compine in the step above 
            _file_infos.emplace_back(std::move(file_info));
        
    }

	M_CLOSEDIR(dir);  
}


ReadDir::ReadDir(const STD_STR& dirname):PathToDir(dirname){init();}
ReadDir::ReadDir(STD_STR&& dirname):PathToDir(std::move(dirname)){init();}

const std::vector<std::string>& ReadDir::GetFiles() {
    
    if(_FilesPath.empty()){
        for(auto&& i : result){
            std::vector<char> buff; //!=================
            Dynamic_strcpy(buff,i.Name.c_str());
            if(i.type == filetype::file) _FilesPath.push_back(VREF(buff));
        }
    }
    return _FilesPath;
}
const std::vector<std::string>& ReadDir::GetFolders() {
    if(_FoldersPath.empty()){
        for(auto&& i : result){
            std::vector<char> buff; //!=================
            Dynamic_strcpy(buff,i.Name.c_str());
            if(i.type == filetype::folder) _FoldersPath.push_back(VREF(buff));
        }
    }
    return _FoldersPath;
}
std::vector<FileInfo>& ReadDir::GetFileInfos() {
    return _file_infos;
}

//void ReadDir::ResetSelection(){
//    for(auto&& i: selection) i=0;
//}

bool ReadDir::find(std::string name){
    GetFiles();  // construct _FilesPath
    GetFolders();// construct _FoldersPath
    auto a = std::find(_FilesPath.begin(),_FilesPath.end(),name);
    auto b = std::find(_FoldersPath.begin(),_FoldersPath.end(),name);
    return (a != _FilesPath.end() || b != _FoldersPath.end());
}