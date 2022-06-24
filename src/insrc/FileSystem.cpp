#include "FileSystem.h"
#include <iostream>
#define MINIZ_INCLUDE_SOURCE
#include "..\outsrc\miniz\miniz.h"
#include <direct.h>
#include "config.h"
#include "ReadDir.h"
#include <unordered_map>
#include "Job.h"

static const char* master_password = "81244b59-0a57-4c68-8a2e-1a398bf03e50";
static const char* infob_password  = "1ea927e3-553d-444d-947f-44852846233d";

#define ASSERT(condition, message) \
    do { \
        if (condition) { \
            std::cerr << "[ERROR]: " << message << '\n'; \
            exit(EXIT_FAILURE); \
        } \
    } while (false)


#define _FREE(x) ( free(x))

//std::queue<std::string>& FileProcessingInof::State();


using uchar = unsigned char;
using uint =  unsigned int;
using ulong =  unsigned long;
using padSize_t = uint;
using RSTR = const char*;


struct InfoBlockContainters{
    std::string sizes;        
    std::string folders;
    std::string folders_index; // folder index
    std::string files;
    std::string files_index;   // file index into the folder index. -1 being the main root 0 being in the first folder and so on
    int err = 0;
    void Reserve(){
        sizes.reserve(512);
        folders.reserve(1024);
        folders_index.reserve(512);
        files.reserve(1024);
        files_index.reserve(512);
    }
};

enum DataBlock{ DB_folder, //folders 
                DB_foleri, //folders index
                DB_file,   //files 
                DB_filei,  //files index
                DB_sizes}; //files sizes


ComProc::ComProc(){}
ComProc::ComProc(const void* data, unsigned int data_size,const std::string& extra_data,int level){ 

    //uint data_size = info_block.sizes[0];
    mz_ulong s = mz_compressBound((mz_ulong)data_size);

    size_t extra_size =  extra_data.size();

    if(level == Compression_levels::NO_COMPRESSION){      
        m_data = malloc(data_size+extra_size);
        memcpy(m_data,extra_data.data(),extra_data.size());
        memcpy((char*)m_data + extra_size, data, data_size);
        final_size = data_size + extra_size;
        good = true;
        return;
    }else{
        m_data = malloc(s+extra_size);
    }

    memcpy(m_data,extra_data.data(),extra_data.size());

    int e = compress2((uchar*)(m_data) + extra_size ,&s,(uchar*)data,data_size,level);
    final_size = s + extra_size;
    if(e || data_size < 20){
        good = false;
        freedat();
    }
    else
        good = true;
}
void ComProc::freedat(){_FREE(m_data);} 


DcomProc::DcomProc(void* Comdata, unsigned long  size,int offset){
    m_size = size; 
    m_data = (uchar*)malloc(m_size);
    int e = mz_uncompress((uchar*)(m_data), &m_size, (uchar*)Comdata + offset , size);
    if(e) {
        good = false;
        freedat();
    }
     // if e is not 0 then we have a proplem
    else good = true;
}
void DcomProc::freedat(){_FREE(m_data);} 


FileWrite::FileWrite(){};
FileWrite::FileWrite(const char* name,std::ios_base::openmode mode){init(name,mode);};
void FileWrite::open(const char* name,std::ios_base::openmode mode ){ init(name,mode);}
void FileWrite::write(const char* dat,size_t s ){ os.write(dat,s); m_data = (void*)dat; m_size=s; }
void FileWrite::clear(){ if(os.is_open()) os.close(); if(!flushed) {  if(path) printf("cleaning unflushed resources '%s'\n",path); std::remove(path);} }
void FileWrite::close( void(*OnClean)()){ flushed=true;  os.flush(); os.close();  if(OnClean) OnClean();}
void FileWrite::writefb( std::filebuf* fb){ os << fb;flushed=true; }
std::ofstream& FileWrite::operator()(){ return os; }
FileWrite::~FileWrite(){clear();}

void FileWrite::init(const char* name,std::ios_base::openmode mode){
    this->path = name; 
    os.open(name,mode);  
    //ASSERT(!os.good(), "Coudn't create: " <<"'" << name << "'. Path may not exist!. ECODE: " << os.rdstate() );
}



FileRead::FileRead() : m_data(nullptr) {}
FileRead& FileRead::operator=(const FileRead& rhs) { m_fnmae=rhs.m_fnmae; m_mode = rhs.m_mode; init(m_fnmae,m_mode,rhs.m_size); return *this;  }
FileRead::FileRead(RSTR fname,std::ios_base::openmode mode,long long in_size, fr_allocator fra): 
m_data(nullptr), m_fnmae(fname), m_mode(mode) {init(m_fnmae,mode,in_size,fra);}

void FileRead::init(RSTR fname,std::ios_base::openmode mode,long long in_size, fr_allocator fra){
    if(st.is_open()) st.close();
    st.open(fname,mode);
    if(!st.good()) return;
    if(in_size<0){
        st.seekg(0,std::ios_base::end);
        m_size = st.tellg();
        st.seekg(std::ios_base::beg);
    }
    else
        m_size = in_size;
    if(fra == nullptr)
        m_data = malloc(m_size);
    else
        fra(&m_data);      
    st.read((char*)m_data,m_size); 
    st.seekg(std::ios_base::beg);
}
std::ifstream& FileRead::operator()(){ return st; }
FileRead::~FileRead() { if(st.is_open()) st.close(); }
void FileRead::freedat(){ if(m_data) _FREE(m_data); }

std::vector<InProcInfo>& FileProcessingInfo::State(){
    std::lock_guard<std::mutex> lock(mx);
    return FileProcessingStates;
}
void FileProcessingInfo::Clear(){
    std::lock_guard<std::mutex> lock(mx);
    FileProcessingStates = std::vector<InProcInfo>();
}
void FileProcessingInfo::Peak(InProcInfo* outdat,int size){
    std::lock_guard<std::mutex> lock(mx);
    for (size_t i = 0; i < size; i++)
    {
        int s = FileProcessingStates.size();
        outdat[ (size-1) - i] =  s > i ? FileProcessingStates.at( (s-1) - i):InProcInfo();
    }
}
std::mutex FileProcessingInfo::mx;
std::vector<InProcInfo> FileProcessingInfo::FileProcessingStates;


std::string Stringed_size(const std::string& ssize){
    long long csize = std::stoll (ssize,nullptr);//!==
    uint fs = csize >= UINT_MAX || csize < 0 ? UINT_MAX : csize;
    auto c_dat = std::string((char*)&fs,sizeof(fs));
    return c_dat; 
}

enum class Cryption_Mode{Encryptio,Decryption};
void BitCryption(void* data, size_t size,const char* passwaord,Cryption_Mode mode){
    unsigned char* cdata = (unsigned char*)data;
    for (size_t n = 0; n < size; n++)
    {
        size_t i1 = n % strlen(passwaord);
        size_t i2 = n % strlen(master_password);
        unsigned char bf1 = passwaord[i1];
        unsigned char bf2 = master_password[i2];
        if(mode == Cryption_Mode::Decryption) std::swap(bf1,bf2);
        cdata[n]  ^=  bf1; 
        cdata[n]  ^=  bf2; 
    }
}

InfoBlockContainters GetFolderInfoBlock_Exc(const wchar_t* _str,std::string rec_count = ""){
    static char Sep_sign = '\0';
    InfoBlockContainters block;block.Reserve();
    int count = 0;
    auto& folder_res  = block.folders;
    auto& file_res    = block.files;
    auto& folderi_res = block.folders_index;   
    auto& filei_res   = block.files_index;
    auto& sizes_res   = block.sizes;           
    ReadDir rd(_str);
    static wchar_t buff[MAX_FILE_NAME];

    if(!rd.is_good()){ // We have a single file
        FileInfo fn(_str,false);
        if(fn.is_good()){
            std::vector<char> buff;
            Dynamic_strcpy(buff,_str);
            char* mp = CutStrToFile(VREF(buff));
            file_res.append(mp);
            file_res.push_back(Sep_sign);
            sizes_res.append(Stringed_size(fn.get_size()));
        }
        else  block.err = 1;
        return block;
    }
    // retrive all files at current path
    for (size_t i = 0; i < rd.GetFiles().size(); i++)
    {
        //file full names
        file_res.append(rd.GetFiles().at(i));
        file_res.push_back(Sep_sign);
        filei_res.append(rec_count.empty() ? "-1":rec_count);
        auto && fv = filei_res.back(); if(fv == '-') fv = Sep_sign; else filei_res += Sep_sign; // can not append a null-t char, we so (+=)

        // file sizes
        // append this folder path with this current file as wchar
        Dynamic_strcpy(buff,rd.GetFiles().at(i).c_str());
        std::wstring wstr = _str; 
        wstr.append(L"\\").append(buff); 
        FileInfo fn(wstr.c_str(),false);
        if(fn.is_good())
            sizes_res.append(Stringed_size(fn.get_size())); 
        else {
            block.err = 1;
            return block;
        }
    }

    // save the root folder at the start of the folder/folderi sextion
    if(rec_count.empty()){      
        const wchar_t* twstr = CutStrToFile(_str);
        static char cbuf[MAX_FILE_NAME];
        Dynamic_strcpy(cbuf,twstr);
        folder_res.append(cbuf); folder_res+=Sep_sign;
        folderi_res.append("-1"); folderi_res+=Sep_sign;
    }

    for (size_t i = 0; i < rd.GetFolders().size(); i++)
    {
       folder_res.append (rd.GetFolders().at(i));
       folder_res.push_back(Sep_sign);

       //construct new path
       Dynamic_strcpy(buff,rd.GetFolders().at(i).c_str()); 
       std::wstring wpt = _str;
       wpt.push_back((M_CHAR)'\\'); 
       wpt.append(buff);

       // start a recursive access then append indexed path (path as an index)
       std::string rec_n = rec_count;
       rec_n.append(std::to_string(i));
       //DEBUG_LOG(i-2 << " " << rec_n);
       rec_n.append("-");
       
       InfoBlockContainters temp_block = GetFolderInfoBlock_Exc(wpt.c_str(),rec_n);
       if(temp_block.err) return temp_block;
       // append indexed path prior to the recursive process
       folderi_res.append(rec_n);
       folderi_res.back() = Sep_sign;

       // retrieve recursive data block then append it to current block
       std::string& fod  = temp_block.folders;
       std::string& fil  = temp_block.files;
       std::string& fodi = temp_block.folders_index;
       std::string& fili = temp_block.files_index;
       std::string& fz   = temp_block.sizes;
       if(fod.size())    folder_res.append(fod);
       if(fil.size())    file_res.append(fil);
       if(fodi.length()) folderi_res.append(fodi);       
       if(fili.length()) filei_res.append(fili);       
       if(fz.length())   sizes_res.append(fz);      
    }

    return block;
}

// provide a path to a folder, and get a block of string containing all of its contents info.
int GetFolderInfoBlock(const wchar_t* path, std::string& out_result){
    InfoBlockContainters block = GetFolderInfoBlock_Exc(path);
    if(block.err) return 0;
    if(!wcslen(path)) return -1;
    if((block.files.empty() && block.folders.empty()) ) return -2;

    if(block.folders.empty()) { 
        out_result.append("**");
        out_result.append(block.files); out_result.append("|");
        out_result.append(block.sizes); out_result.append("*");
        return 2;
    }

    out_result.append("*");
    out_result.append(block.folders);       out_result.append("|");
    out_result.append(block.folders_index); out_result.append("|");
    out_result.append(block.files);         out_result.append("|");
    out_result.append(block.files_index);   out_result.append("|");
    out_result.append(block.sizes);         out_result.append("*");
    
    return 1;
}

// provide a path to a folder, and get a block of string containing all of its contents info.
int GetFolderInfoBlock(const char* path, std::string& out_result){
    std::vector<M_CHAR> wbuf;
    Dynamic_strcpy(wbuf,path);
    return GetFolderInfoBlock(&wbuf[0],out_result);
}


//extracts data into vectors of (const char*) within a BlockSection using the returned string from GetFolderInfoBlock(). This string MUST presist. 
int Extract_info_block(const char* infob,BlockSection& c_ptrs){
    //if(infob.empty()) return 0;
    if(infob[0] != '*') return 0;

    if(infob[1] == '*'){
        c_ptrs.files.push_back(&infob[2]);
        const char* ptr=&infob[2];
        c_ptrs.sizes.push_back(  *(unsigned int*)(ptr + strlen(ptr)+2) );
        return 2;
    }

    int type = DB_folder;
    int starcount=0;
    for (size_t i = 0; starcount < 2; i++)
    {
        if(infob[i] == '*') starcount++;
        if(type > DB_sizes || starcount > 2) return 0;
        if(type == DB_sizes){
            for (size_t j = 0; j < c_ptrs.files.size(); j++)
            {
                const char* ptr = &infob[i] + 1 + (j*4);
                c_ptrs.sizes.push_back(*((unsigned int*)ptr));
            }
            break;
        }

        if(infob[i] == '\0' || i == 0){
            int offset = 1;
            if(infob[i+1] == '|'){ offset = 2; type++;  }
            switch (type)
            {
                case DB_folder: c_ptrs.folders.emplace_back(&infob[i] + offset);  break;
                case DB_foleri: c_ptrs.foldersi.emplace_back(&infob[i] + offset); break;
                case DB_file:   c_ptrs.files.emplace_back(&infob[i] + offset);    break;
                case DB_filei:  c_ptrs.filesi.emplace_back(&infob[i] + offset);   break;             
            }
            
        }     
    }
    return 1;
}

// construct a full file path for each file in the BlockSection. Return 1 success.
int ConstructFilePaths(const BlockSection& bs, std::vector<std::string>& out_file_paths, std::string root){

    auto&& folders  = bs.folders;
    auto&& foldersi = bs.foldersi;
    auto&& files    = bs.files;
    auto&& filesi   = bs.filesi;
    if(folders.size() < 0 || foldersi.size() < 0 || files.size() < 0 || filesi.size() < 0 )
        return 0;

    // maping folders index to folders names
    std::unordered_map<std::string,std::string> maped_paths;
    for (size_t i = 0; i < folders.size(); i++)
    {
        maped_paths[foldersi[i]] = folders[i];
    }
    
    std::string RootFolder = root.empty() ? maped_paths["-1"] : root; //! we must have a root folder
    out_file_paths.reserve(filesi.size());
    for(auto&&i:out_file_paths) i.reserve(MAX_PATH);
    // construct
    for (size_t i = 0; i < filesi.size(); i++)
    {
        out_file_paths.push_back(RootFolder);
        auto&& ls = out_file_paths.back();
        const char* sc = filesi.at(i);
        if(*sc == '-' ) {ls.append("\\").append(files.at(i)); continue;}

        int strl =  strlen(filesi[i]);
        for (size_t j = 0; j <= strl; j++)
        {
            const char* mc = sc + j;
            if(*mc == '\0') {
                ls.append("\\").append(maped_paths[std::string(sc,j)]);
                continue;
            }
            if(*mc == '-')
                ls.append("\\").append(maped_paths[std::string(sc,j)]);
        }  
        ls.append("\\").append(files.at(i));
    }
    return 1;
}


// Creates folders in the hardisc based on the BlockSection folders data. Return 1 success.
int CreateSystemFolders(const BlockSection& bs,const std::string root_extention){

    auto&& folders  = bs.folders;
    auto&& foldersi = bs.foldersi;
    if(folders.size() < 0 || foldersi.size() < 0)
        return 0;

    std::unordered_map<std::string,std::string> maped_paths;

    for (size_t i = 0; i < folders.size(); i++)
    {
        maped_paths[foldersi[i]] = folders[i];
    }
    
    std::string RootFolder = root_extention + maped_paths["-1"]; //! we must have a root folder
    mkdir(RootFolder.c_str()); // construct a root folder
    // construct
    for (size_t i = 0; i < folders.size(); i++)
    {
        std::string maked_dir =  RootFolder;

        const char* sc = foldersi.at(i);
        if(*sc == '-' ) { continue;}

        int strl =  strlen(foldersi[i]);
        for (size_t j = 0; j <= strl; j++)
        {
            const char* mc = sc + j;
            if(*mc == '\0') {
                maked_dir.append("\\").append(maped_paths[std::string(sc,j)]);
                continue;
            }
            if(*mc == '-')
                maked_dir.append("\\").append(maped_paths[std::string(sc,j)]);
        }  
        mkdir(maked_dir.c_str());
    }
    return 1;
}

// Creates folders in the hardisc based on the BlockSection folders data. Return 1 success.
int ConstructFolders(const BlockSection& bs, std::vector<std::string>& out_folderPaths,const std::string root_extention){

    auto&& folders  = bs.folders;
    auto&& foldersi = bs.foldersi;
    if(folders.size() < 0 || foldersi.size() < 0)
        return 0;

    std::unordered_map<std::string,std::string> maped_paths;

    for (size_t i = 0; i < folders.size(); i++)
    {
        maped_paths[foldersi[i]] = folders[i];
    }
    
    std::string RootFolder = root_extention + maped_paths["-1"]; //! we must have a root folder
    //mkdir(RootFolder.c_str()); // construct a root folder
    // construct
    for (size_t i = 0; i < folders.size(); i++)
    {
        out_folderPaths.push_back (RootFolder);
        auto & maked_dir = out_folderPaths.back();
        const char* sc = foldersi.at(i);
        if(*sc == '-' ) { continue;}

        int strl =  strlen(foldersi[i]);
        for (size_t j = 0; j <= strl; j++)
        {
            const char* mc = sc + j;
            if(*mc == '\0') {
                maked_dir.append("\\").append(maped_paths[std::string(sc,j)]);
                continue;
            }
            if(*mc == '-')
                maked_dir.append("\\").append(maped_paths[std::string(sc,j)]);
        }  
        //mkdir(maked_dir.c_str());
    }
    return 1;
}

int GetInfoBlockSize(const char* infob){
    size_t i = 2;

    if(infob[0] != '*') return 0;
    if(infob[1] == '*'){
        //short c_start = 2
        //!========================================
        for (;infob[i] != '|'; i++) { }
        i += 2 + sizeof(unsigned int);
        if (infob[i-1] != '*') return 0;
        return i;       
    }
    int I_count = 0;
    int Zero_count = 0;
    while (I_count != 4)
    {
        if(infob[i] == '*') return 0;
        if(infob[i] == '|')
            I_count++;
        if(I_count == 2 && infob[i] == '\0')
            Zero_count++;
        i++;
    }
    i += sizeof(unsigned int) * Zero_count;
    if (infob[i] != '*') return 0;
    return i +1;
}

template<class T>
size_t accumulate(const std::vector<T>& nums){
    size_t com = 0;
    for(auto&& i : nums) com += i;
    return com;
}

int DynamicCompress(std::string path, std::string out_file_name,std::string passport, int level){
    //!maybe store the level state as well
    static char buff[MAX_FILE_NAME];

    std::string infob;
    infob.reserve(MAX_PATH);
    Normal_stat("Preparing files",0.0f);
    int file_type = GetFolderInfoBlock(path.c_str(),infob);
    if(file_type <= 0){
        Error_stat(file_type == 0 ? "file or folder name error" : file_type == -1 ? "path error" : "empty folder");
        return 0; // no data block fail
    }

    while(LEACH::Pause){}
    if(LEACH::cancele) return -15;

    BlockSection iblock;
    Normal_stat("Extracting Info Block Data",.0f);
    int res = Extract_info_block(infob.data(),iblock);
    size_t total_szie = accumulate(iblock.sizes);

    float sum = iblock.files.size() + 3.0f +  (iblock.files.size() /50.0f);
    float a = 100.0f / sum;

    if(total_szie >= UINT_MAX){
        Error_stat("Failed. limited Data size exceede");
        return -10; // this size is not supported
    }

    void* all_data = malloc(total_szie);
    //LEACH::TotalProg_prog = total_szie;
    if(file_type == 1){ // files (path to folder)
        std::vector<std::string> files_paths; //! full path error [ERROR]: Coudn't open 'BinPack-main\BinPack-main\README.md'
        //base += a;
        Normal_stat("Constructing file paths",a);
        ConstructFilePaths(iblock, files_paths, path);
        int acom_sizes =0;
        for (size_t i = 0; i < files_paths.size(); i++)
        {
            while(LEACH::Pause){}
            if(LEACH::cancele){free(all_data); return -15;}
            const auto alocater = [=](void ** data){
                *data = ((char*)all_data + acom_sizes);
            };
            Normal_stat(iblock.files[i],a * (i+1));
            FileRead fr(files_paths[i].c_str(),BIN_READ,iblock.sizes.at(i),alocater);
            if(!fr().good() || fr().bad()){
                free(all_data);
                Error_stat(("Failure while reading data from: "+ files_paths[i]).c_str());
                return -1; // fail while reeding data
            }
            acom_sizes += fr.m_size;
        }
    }
    float s = iblock.files.size();
    if(file_type == 2){ //single file
        sprintf(buff,"Reading: %s", path.c_str());
        Normal_stat(buff,a * s);
        FileRead fr(path.c_str(),BIN_READ,iblock.sizes.at(0),[=](void ** data){
            *data = (char*)all_data;
        });   
        if(!fr().good()){
            free(all_data);
            sprintf(buff,"Failure while reading data from: %s", path.c_str());
            Error_stat(buff);
            return -1; // fail while reeding a file (might be a wide char)
        }  
    }

    switch (level)
    {
        case NO_COMPRESSION:        infob.insert(0,"0"); break;
        case DEFAULT_COMPRESSION:   infob.insert(0,"1"); break;
        case BEST_SPEED:            infob.insert(0,"2"); break;
        case BEST_COMPRESSION:      infob.insert(0,"3"); break;
    }
    Normal_stat("Compressing data...",a * (s + 1));
    ComProc compresor(all_data,total_szie,infob,level);
    if(!compresor.Is_good()){
        free(all_data);
        Error_stat("Failure while compressing");
        return -2; // compression error
    }
    while(LEACH::Pause){}
    if(LEACH::cancele){ free(all_data);return -15;}

    Normal_stat("Writing data...",a *(s + 2));


    FileWrite fwr(out_file_name.c_str(),BIN_WRITE); //! delete file after cancelations
    if(!fwr().good()){
        free(all_data);
        compresor.freedat();
        Error_stat("Failure while writing");
        return -3; // error while writing 
    }

    if(!passport.empty()){
        Normal_stat("Encrypting", a *(s + 3));
        BitCryption(compresor.m_data,compresor.final_size,passport.c_str(),Cryption_Mode::Encryptio);
    }

    fwr.write((const char*)compresor.m_data,compresor.final_size);
    fwr.close();
    free(all_data);

    while(LEACH::Pause){}
    if(LEACH::cancele) { remove(out_file_name.c_str()); return -15;}

    compresor.freedat();
    Done_stat("Done");
    return 1;
}


int DynamicDecompress(std::string target,std::string dest_folder,std::string passport){
    static M_CHAR buff[MAX_PATH];
    bool folder_was_overrided = false;

    Dynamic_strcpy(buff,target.c_str());
    FileInfo fn(VREF(buff),false);
    if(!fn.is_good()){
        Error_stat("File unvalid");
        return -4;
    }
    Normal_stat("Preparing files",0.0f);

    FileRead fr(target.c_str(),BIN_READ,fn.win_size);
    if(!fr().good()){ 
        Error_stat("Couldn't open file");
        return 0 ;
    }
    fr().close();

    Normal_stat("Decrypting",0.0f); //! faster than usual decruption
    if(!passport.empty())
        BitCryption(fr.m_data,fn.win_size,passport.c_str(),Cryption_Mode::Decryption);
    
    char com_level = *(char*)fr.m_data;
    void* infpb = (char*)fr.m_data + 1;

    int infoBsize = GetInfoBlockSize((char*)infpb);
    if(infoBsize == 0) {  
        Error_stat( passport.empty() ? "Couldn't recognize file" :"Wrong password or corrupted file." );
        return -1; // info block doesn't exist or coudn't be recognized..
    }

    BlockSection iblock;
    int file_type = Extract_info_block((char*)infpb,iblock); 

    float sum = iblock.files.size() + 4.0f;
    float a = 100.0f / sum;

    std::string outfpath;
    if(strlen(dest_folder.c_str())){   
        folder_was_overrided = mkdir(dest_folder.c_str());
        outfpath = std::string(dest_folder) + "\\";
    }
    //DEBUG_LOG("folder was overrided ? " << std::boolalpha << folder_was_overrided);
    unsigned int total_szie = accumulate(iblock.sizes);

    void* target_data = nullptr;
    if(com_level != '0'){ 
        Normal_stat("Decompressing", a);
        DcomProc decompressor(infpb,total_szie,infoBsize);
        target_data = decompressor.m_data;
        if(!decompressor.Is_good()){
            if(!folder_was_overrided) RemoveDir(dest_folder);
            Error_stat("Failed while decompressing");
            return -2;
        }
    }
    else{    // means we have an uncompressed data
        Normal_stat("Unpacking files",a);
        target_data = (char*)infpb + infoBsize;
    }
    
    while(LEACH::Pause){}
    if(LEACH::cancele) { if(!folder_was_overrided) RemoveDir(dest_folder); return -15;}
    //if(LEACH::cancele) { return -15;}

    // we have a file ------------VVVV
    if(file_type==2){
        Normal_stat("writing files",a*2);
        FileWrite fw( (outfpath + iblock.files[0]).c_str() ,BIN_WRITE);
        if(!fw().good()){
            Error_stat("Failed while writing files");
            if(!folder_was_overrided) RemoveDir(dest_folder);
            return -3;
        }
        fw.write( (char*)target_data, iblock.sizes[0]);
        fw.close();
        Done_stat("Done");
        return 1;
    }

    Normal_stat("Preparing containing folders",a*3); 
    // we have a folder ----------VVVV
    CreateSystemFolders(iblock,outfpath);
    std::vector<std::string> fullpaths;
    ConstructFilePaths(iblock,fullpaths);

    unsigned int com_sizes = 0;
    for (size_t i = 0; i < fullpaths.size(); i++)
    {
        while(LEACH::Pause){}
        if(LEACH::cancele) { if(!folder_was_overrided) RemoveDir(dest_folder); return -15;}

        std::string to_p =  outfpath + fullpaths[i];
        Normal_stat(iblock.files[i], a * (i + 4));
        FileWrite fw(to_p.c_str(),BIN_WRITE); //! ================================ delete after cancelations
        if(!fw().good()){
            Error_stat("Failed while writing files");
            if(!folder_was_overrided) RemoveDir(dest_folder);
            return -3;
        }
        fw.write((char*)target_data + com_sizes, iblock.sizes[i]);
        com_sizes+=iblock.sizes[i];
        fw.close();
    }

    //free data when we used DcomProc (in case wse are dealing with compressed data)
    if(com_level != '0')
       free(target_data);
    Done_stat("Done");
    return 2;
}


int copyDir(const char* path, const char* to){
    std::string infob;
    int file_type = GetFolderInfoBlock(path,infob);
    if(file_type<=0) return 0; 
    BlockSection iblock;
    int res = Extract_info_block(infob.data(),iblock);
    mkdir(to);
    std::string outfdr = std::string(to) + "\\";
    std::vector<std::string> files_paths; //! full path error [ERROR]: Coudn't open 'BinPack-main\BinPack-main\README.md'
    ConstructFilePaths(iblock, files_paths);
    CreateSystemFolders(iblock,outfdr.c_str());

    std::string lfs = path; 
    if(!CutStrToPath(&lfs[0])) // full file name case
    { 
        lfs.clear();
    }else{
        lfs = std::string(lfs.data());
        lfs += '\\';
    }
    for (size_t i = 0; i < files_paths.size(); i++)
    {
        std::string str = lfs + files_paths[i];
        FileRead fr(str.c_str(),BIN_READ,iblock.sizes.at(i));
        FileWrite fwr( (outfdr + files_paths[i]).c_str() ,BIN_WRITE);
        fwr.write((const char*)fr.m_data,fr.m_size);
        fwr.close();
        fr.freedat();
    }
    return 1; 
}
int copyFile(const char* path, const char* to){
    std::string str = path;
    char* p = CutStrToFile(&str[0]);
    std::ifstream src(path, BIN_WRITE);
    if(!src.good()) return 0;
    std::string top = to;
    if(strlen(to))
        top.append("\\");
    top.append(p);
    std::ofstream dest((top).c_str(), BIN_WRITE);
    if(!dest.good()) return 0;
    dest << src.rdbuf();
    dest.close();
    src.close();
    return 1;
}


int RemoveDir(const std::string& path){
    if(!path.size()) return 0;
    std::string infob;
    int file_type = GetFolderInfoBlock(path.c_str(),infob);
    if(file_type <= 0) return 0;
    BlockSection iblock;
    int res = Extract_info_block(infob.data(),iblock);

    std::string lfs = path; 
    if(!CutStrToPath(&lfs[0])) // full file name case //!sort this problem
    { 
        lfs.clear();
    }else{
        lfs = std::string(lfs.data()); // reassign to remove dead chars 
        lfs+="\\";
    }

    std::vector<std::string> files_paths; 
    ConstructFilePaths(iblock, files_paths,path);
    std::vector<std::string> folder_paths;
    ConstructFolders(iblock, folder_paths,lfs);
    std::reverse(folder_paths.begin(),folder_paths.end());
    for(auto&&I:files_paths)  {remove(I.c_str());}
    for(auto&&I:folder_paths) {rmdir(I.c_str());}
    return 1;
}

std::string To_temp_path(){
    const char* temp_p = std::getenv("TEMP");
    if(!temp_p) 
        return {};
    std::string tempdist = temp_p;
    return tempdist.append("\\tsh_tempa");
}

int CompressCollection(std::vector<std::string> paths, std::string dist_folder, std::string outfname,std::string passport){ // paths to folders or filse
    if(dist_folder.empty()) return 0;

    std::string temp_path = To_temp_path();
    if(!temp_path.size()) {
        Error_stat("temp path not found");
        return -1; // temp path not found
    }
    

    //std::string tempdist = temp_p;
    std::string Base = temp_path;
    RemoveDir(Base.c_str());
    //DEBUG_LOG(Base);
    Normal_stat("Preparing containing folder",0.0f);
    mkdir(Base.c_str());
    temp_path.append("\\").append(dist_folder);
    mkdir(temp_path.c_str());

    
    Normal_stat("Copying files to containing folder",0.0f);
    for (size_t i = 0; i < paths.size(); i++)
    {
        std::vector<wchar_t> buff;
        Dynamic_strcpy(buff,paths[i].c_str());
        FileInfo fn(VREF(buff));

        if(fn.get_type() == "Directory"){
            if(!copyDir(paths[i].c_str(),temp_path.c_str())){
                Error_stat("failed while Copying a folder");
                return -2;
            }
        }
        if(fn.get_type() == "File"){
            
            if(!copyFile(paths[i].c_str(),temp_path.c_str())){
                Error_stat("failed while Copying a file");
                 return -3;
            }
        }
    }

    int res = DynamicCompress(temp_path.c_str(),outfname,passport);
    RemoveDir(Base);
    if(res > 0)
        return 1;
    return -10;
}

bool Is_path_valid(const char* path){
    wchar_t buff[MAX_PATH];
    Dynamic_strcpy(buff,path);
    FileInfo is_path_valid (VREF(buff),false); 
    return is_path_valid.is_good();
}
bool Is_path_valid(const wchar_t* path){ FileInfo is_path_valid (path,false);  return is_path_valid.is_good(); }


//!when file or folder couldn't be read, eject
void recfolders(const std::wstring& path){


}