#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <functional>
#include <mutex>

struct BlockSection{
    std::vector<const char*> folders;
    std::vector<const char*> foldersi;
    std::vector<const char*> files;
    std::vector<const char*> filesi;
    std::vector<unsigned int> sizes;
};

// Compression levels: 0-9 are the standard zlib-style levels, 10 is best possible compression (not zlib compatible, and may be very slow), MZ_DEFAULT_COMPRESSION=MZ_DEFAULT_LEVEL.
enum Compression_levels { NO_COMPRESSION = 0, BEST_SPEED = 1, BEST_COMPRESSION = 9, UBER_COMPRESSION = 10, DEFAULT_LEVEL = 6, DEFAULT_COMPRESSION = -1 };


#define BIN_WRITE std::ios::out | std::ios::binary
#define TXT_WRITE std::ios::out | std::ios::trunc
#define BIN_READ  std::ios::in | std::ios::binary


// provide a path to a folder, and get a block of string containing all of its contents info. return 0 in failure, 1 folder, 2 file 
int GetFolderInfoBlock(const char* path, std::string& out_result);
int GetFolderInfoBlock(const wchar_t* path, std::string& out_result);

//extracts data into arraies within a BlockSection object using a InfoBlock. infob MUST presist while BlockSection is relevent.
// return 0 in failure, 1 folder, 2 file 
int Extract_info_block(const char* infob,BlockSection& c_ptrs);

// construct a full file path for each file into an arry of strings from a BlockSection. Return 0 on failure and 1 on success.
int ConstructFilePaths(const BlockSection& bs, std::vector<std::string>& out_file_paths, std::string root = "");
int ConstructFolders(const BlockSection& bs, std::vector<std::string>& out_folderPaths,const std::string root_extention);
// Creates folders in the disk based on a BlockSection folders data. Return 0 on failure and 1 on success.
int CreateSystemFolders(const BlockSection& bs,const std::string root_extention = "_");
int GetInfoBlockSize(const char* infob);

int DynamicCompress(std::string path, std::string out_file_name,std::string passport,int level=DEFAULT_COMPRESSION);      // the file name of the compressed data
int DynamicDecompress(std::string target, std::string dest_folder,std::string passport);    // the destination folder the decompression will go into
int CompressCollection(std::vector<std::string> paths, std::string dist_folder, std::string outfname,std::string passport);

int copyDir(const char* path, const char* to);
int copyFile(const char* path, const char* to);
int RemoveDir(const std::string& path);

bool Is_path_valid(const char* path);
bool Is_path_valid(const wchar_t* path);

std::string To_temp_path();

using fr_allocator = std::function<void(void** data)>;


struct FileRead{
    void* m_data;
    size_t m_size = 0;
    const char* m_fnmae;
    std::ifstream st;
    std::ios_base::openmode m_mode;

    FileRead();
    FileRead& operator=(const FileRead& rhs);
    FileRead(const char* fname,std::ios_base::openmode mode,long long in_size = -1, fr_allocator= nullptr);
    void init(const char* fname,std::ios_base::openmode mode,long long in_size = -1, fr_allocator = nullptr);
    std::ifstream& operator()();
    ~FileRead();
    void freedat();
};


struct FileWrite{
    const char* path;
    std::ofstream os;
    size_t m_size;
    bool flushed=false;
    void * m_data;
    std::ios_base::openmode Mode;
    FileWrite();
    FileWrite(const char* name,std::ios_base::openmode mode);
    void open(const char* name,std::ios_base::openmode mode );
    void write(const char* dat,size_t s );
    void clear();
    void close( void(*OnClean)() = nullptr );
    void writefb( std::filebuf* fb);
    std::ofstream& operator()();
    ~FileWrite();
    private:
    void init(const char* name,std::ios_base::openmode mode);
};



struct ComProc{
    unsigned long final_size;
    void* m_data;
    ComProc();
    ComProc(const void* data, unsigned int data_size, const std::string& extra_data = "",int level = DEFAULT_COMPRESSION);
    void freedat();
    inline const bool Is_good() const {return good;}
    private: 
    bool good=false;
};

struct DcomProc{ 
    void* m_data=nullptr; 
    unsigned long  m_size;

    DcomProc(void* Comdata, unsigned long  size,int offset = 0);
    void freedat(); 
    inline const bool Is_good() const {return good;}
    private: 
    bool good=false;
};


struct InProcInfo {InProcInfo(const char* stat = "",float prce=0.0f):stat(stat),prce(prce) {}  std::string stat; float prce;};
struct FileProcessingInfo{
    static std::vector<InProcInfo>& State();
    static void Clear();
    static void Peak(InProcInfo* outdat,int size);
    private:
    static std::mutex mx;
    static std::vector<InProcInfo> FileProcessingStates;
};

#define Normal_stat(a,b)\
    FileProcessingInfo::State().push_back({a,b});
#define Error_stat(a)\
    FileProcessingInfo::State().push_back({a,-100.0f});
#define Done_stat(a)\
    FileProcessingInfo::State().push_back({a,100.0f});

namespace LEACH{
    const unsigned int Get_TotalProg_prog();
    const unsigned int Get_Task_prog();
    const bool Is_Progress();
} 
