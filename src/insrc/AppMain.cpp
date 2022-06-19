#define BP_INSER_RESOURCES

#include "config.h"

#include <algorithm>
#include <unordered_map>

#include "FileSystem.h"
#include "FontData.h"
#include "app_window.h"
#include "ReadDir.h"
#include "..\outsrc\tinydialog\tinyfiledialogs.h"
#include "BitmapData.h"
#include "FileInfo.h"
#include "Job.h"
#include "LEACH.h"
#include <chrono>


// glyph index
#define ENC_INDEX   1337
#define DEC_INDEX   1339
#define ENTER_INDEX 24
#define FILED_INDEX 406
#define TOUP_INDEX  25
#define TRASH_INDEX 206
#define X_INDEX 7



#define LOGO_SIZE 50.0f
#define FONT_SIZE 23.0f
#define FONT_GLOBAL_SIZE (ImGui::GetIO().FontGlobalScale * ImGui::GetFont()->FontSize * ImGui::GetFont()->Scale * ImGui::GetCurrentWindow()->FontWindowScale)
#define debug_float static float testf = 0.0f;                \
                     ImGui::Begin("test float");              \
                     ImGui::DragFloat("testing",&testf,0.01f);\
                     ImGui::End();                            

#define POPUP_FLAGS ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |ImGuiWindowFlags_NoSavedSettings

#define _style   (ImGui::GetStyle())
#define _io      (ImGui::GetIO())
#define _guiWin  (ImGui::GetCurrentWindow())
#define _context (ImGui::GetCurrentContext())


ImGuiStyle MyGuiStyle(); 
Job<int> Data_ProcessingJob;
//std::queue<std::string> FileProcessingStates;

static ImFont* main_font,*small_font;
static BitmapData to_up_icon,fileD_icon,enter_icon, encrypt_icon,decrypt_icon,X_icon,trash_icon;
//static std::string passStr;

//Use these buffer for read/write process
static std::vector<M_CHAR> w_folder_buff;
static std::vector<M_CHAR> w_folder_dist_buff;

static std::vector<char> folder_buff;
static std::vector<char> file_extension;
//name_buff : name of the enctypted file in case of encryption and name of the folder in case of decryption.
static std::vector<char> password_buff,folder_dist_buff,name_buff; 
static int Selection_double_click;
static ReadDir read_dir;


struct ProgBarsDep{
    float prevPr = 0.0f,curPr = 0.0f, delta=0.0f; 
    float MaxPeak=1.0f;
    bool animate = true;
    int values_offset = 0;
    double refresh_time = 0.0;
    //float delta = 0.0f;
}pbd;

struct TS_Context{
    int comp_item = 2;
    //elastic processing; is the process of compressing or decompressing.
    int elasproc_stat_size=0; //size 
    bool is_elasprocessing=false;
    float bars[90] = {}; 
    const char* popup_name;
    bool Del_af_Dec = false;
    bool overwrite_file = false;
    ImVec2 clickedPos;
    TS_Context(){
        memset(bars,0,IM_ARRAYSIZE(bars) * sizeof(float));
    }
}ts_context;

namespace StopWatch
{
    using ts_clock = std::chrono::steady_clock; 
    ts_clock::time_point start,counting;
    char strtime[12];

    void Reset(){counting, start = ts_clock::now();}
    void Start(){
        start = ts_clock::now();
        Reset(); // make sure counter reseted at start
    }
    void Counting(){
        counting = ts_clock::now();
    }
    unsigned long Seconds(){
        return std::chrono::duration_cast<std::chrono::seconds>(counting - start).count();
    }
    unsigned long Minuts(){
        return std::chrono::duration_cast<std::chrono::minutes>(counting - start).count();
    }
    unsigned long Hours(){
        return std::chrono::duration_cast<std::chrono::hours>(counting - start).count();
    }
    
    const char* Get_Strtime(){
        const char* zt = "00";
        unsigned int sec = Seconds() % 60, min = Minuts() % 60, hour = Hours();
        memset(strtime,0,IM_ARRAYSIZE(strtime));
        char buff[12];
        itoa(hour,strtime,10);
        strcat(strtime,":");
        itoa(min,buff,10);
        strcat(strtime,zt+strlen(buff));
        strcat(strtime,buff);
        strcat(strtime,":");
        itoa(sec,buff,10);
        strcat(strtime,zt+strlen(buff));
        strcat(strtime,buff);
        return strtime;
    } 

};

//std::vector<std::string> Error_stack; // push errors here

void Clearbuffers(){
    memset(VREF(password_buff),0,password_buff.size());
    memset(VREF(folder_dist_buff),0,folder_dist_buff.size());
    memset(VREF(name_buff),0,name_buff.size());
    memset(VREF(w_folder_dist_buff),0,w_folder_dist_buff.size() * sizeof(M_CHAR));
}


// on new path
void OnNewPath(){
    //! when we have last \\ then don't emmit it
    // remove last letter slashes. getting Icons function doesn't like it
    for (M_CHAR* i = VREF(w_folder_buff) + M_strlen(VREF(w_folder_buff)) - 1; *i == (M_CHAR)'\\'  ||  *i == (M_CHAR)'/' ; i--) {*i = (M_CHAR)'\0';}

    int vc_size = M_strlen (VREF(w_folder_buff));
    int c_size = strlen (VREF(folder_buff));
    Dynamic_strcpy(folder_buff, VREF(w_folder_buff));//          
    read_dir = ReadDir(VREF(w_folder_buff));
}

void on_selected(std::function<void(int i)> func) {
    for (size_t i = 0; i < read_dir.GetFileInfos().size(); i++)
    {
        if(read_dir.GetFileInfos().at(i).Selection == true) {
            func(i);
        }
    }
};

std::vector<std::string> GetSelectedFilesNames(){
    std::vector<std::string> res;
    on_selected([&res](int i) {
        std::string final_namel;
        auto& selected_file_name = read_dir.GetFileInfos().at(i);// weak file names    
        final_namel = selected_file_name.display_name();
        if (selected_file_name.get_name_type() == "Shortcut")
            final_namel.append(".lnk");
        res.push_back(final_namel);
    });
    return res;
}

void List(){    
    const int columns_number = 4;
    static float columns_sizes[columns_number-1]; // -1 to skip first column
    float f_gap = FONT_GLOBAL_SIZE*1.430f;
    ImVec2 crPos = ImGui::GetCursorPos();
    ImGui::Dummy(ImVec2(0,f_gap));

    ImGui::SetCursorPos(ImVec2 (ImGui::GetCursorPosX(),ImGui::GetCursorPosY() -  ImGui::GetStyle().ItemSpacing.y));
    ImVec2 avsize = ImGui::GetContentRegionAvail();

    ImGui::BeginChildFrame(ImGui::GetID("list"), avsize, ImGuiWindowFlags_NoMove);
    ImGui::Columns(4);
    ImGuiListClipper clipper; // Also demonstrate using the clipper for large list
    if(read_dir.GetFileInfos().empty()) {
        for (size_t i = 0; i < columns_number-1; i++)
        {
            ImGui::SetColumnOffset(i+1,315.0f + (i*90.0f));
        }       
    }else{
        clipper.Begin(read_dir.GetFileInfos().size());
        //DEBUG_LOG(read_dir.GetFileInfos().size());
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++){
                for (int j = 0; j < columns_number; j++)
                {
                    FileInfo &fiv = read_dir.GetFileInfos().at(i);
                    const BitmapData* icondat = fiv.get_file_icon();
                    ImVec2 cp = ImGui::GetCursorPos();
                    if(!j){
                        ImGui::SetCursorPosY(cp.y - -0.210f * FONT_GLOBAL_SIZE);
                        
                        ImGui::Image((ImTextureID)(intptr_t)icondat->gpu_id, ImVec2(icondat->width, icondat->hight));
                        ImGui::SameLine();
                        
                    }
                    // y offset for the icons 
                    ImGui::SetCursorPos(ImVec2( !j ? cp.x + icondat->width * 1.5f : cp.x,cp.y));
                    int selectable_flages = ImGuiSelectableFlags_SpanAllColumns|ImGuiSelectableFlags_AllowItemOverlap;
                    const char* labels[columns_number] = {fiv.display_name().c_str(), fiv.get_type().c_str(),fiv.get_size().c_str(),fiv.get_modified_time().c_str() };
                    //static std::vector<char> buff; Dynamic_strcpy( buff, read_dir.result.at(i).Name.c_str() );
                    //const char* labels[columns_number] = {&buff[0], fiv.get_type().c_str(),fiv.get_size().c_str(),fiv.get_modified_time().c_str() };
                    if (!ImGui::IsPopupOpen("###popup") && !j && ImGui::Selectable(labels[j], fiv.Selection,selectable_flages))
                    {
                        if (!ImGui::GetIO().KeyCtrl){    // Clear selection when CTRL is not held
                            ts_context.clickedPos = ImGui::GetCursorScreenPos();
                            on_selected([](int i) { read_dir.GetFileInfos().at(i).Selection = false; });
                        }
                        fiv.Selection ^= 1;
                    }

                    if(j>0){
                        ImGui::TextUnformatted(labels[j]);
                    }
                    ImGui::NextColumn();
                    skip:
                    if(j<columns_number-1) columns_sizes[j] = ImGui::GetColumnOffset(j+1); // record columns offset in columns_sizes
                }
            }
        }
    }
    ImGui::Columns(1);
    ImGui::EndChildFrame();

    ImVec2 ncursPos = ImGui::GetCursorPos();

    ImGui::SetCursorPos(crPos);
    ImVec2 child_size = ImGui::GetContentRegionAvail();
    ImGui::BeginChildFrame(ImGui::GetID("1"), ImVec2(child_size.x,f_gap), ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    ImGui::Columns(4,(const char*)0,false);
    for (size_t i = 0; i < 3; i++)
    {
       ImGui::SetColumnOffset(i+1,columns_sizes[i]);
       /* code */
    }
    ImGui::Text("File"); ImGui::NextColumn();
    ImGui::Text("Type"); ImGui::NextColumn();
    ImGui::Text("Size"); ImGui::NextColumn();
    ImGui::Text("Modified"); 
    ImGui::Columns(1);
    ImGui::EndChildFrame();
    ImGui::SetCursorPos(ncursPos);
}


ImFont* LoadFont(size_t fontdatasize, const void* fontdata, float fontsize){
    auto io = ImGui::GetIO();
    void* mainfontData1 = malloc(fontdatasize);
    if (mainfontData1) memcpy(mainfontData1, fontdata, fontdatasize);
    return io.Fonts->AddFontFromMemoryTTF(mainfontData1, fontdatasize, fontsize);
}


ImVec2 cal_logo_size(const BitmapData& lim ){
    float yh = lim.hight;
    float ryh = yh - yh;
    float xw = lim.width;
    float rxw = xw - yh; 
    
    return ImVec2(rxw + FONT_GLOBAL_SIZE,ryh + FONT_GLOBAL_SIZE);
}

float X_midpos_To_Width(float xPos,float dist_width, float src_width){
    return xPos + ((dist_width +_style.FramePadding.x*2.0f) - src_width)/2.0f;
}

void FramedDataRegion(const char* label, ImGuiID id, ImVec2 size, std::function<void()> func,bool linked = false){
    //ImVec2 avSpac;
    if(!linked)
        ImGui::Dummy(ImVec2(0.0f,_style.ItemSpacing.y));

    ImVec2 cpos = ImGui::GetCursorPos();
    ImVec2 secpos = ImGui::GetCursorScreenPos();
    auto* dl = ImGui::GetWindowDrawList(); 
    
    //ImGui::SetCursorPosY(cpos.y + FONT_GLOBAL_SIZE * 0.7f);
    //ImVec2 regi = ImGui::GetCursorScreenPos();//!-tfbnftnfn
    ImGui::BeginChildFrame(id,size);
    ImGui::PushClipRect(ImVec2(secpos.x,secpos.y + FONT_GLOBAL_SIZE/ 2.0f),ImVec2(secpos.x + 200000.0f,secpos.y + 20000.0f),true);
    ImGui::Dummy(ImVec2(0,FONT_GLOBAL_SIZE / 2.0f));
    if(func) func();
    ImVec2 cpos3 = ImGui::GetCursorPos();
    ImGui::PopClipRect();
    ImGui::EndChildFrame(); 
    
    ImVec2 TexPos = ImVec2 (secpos.x + _style.ItemSpacing.x * 1.0f,secpos.y - (_style.ItemSpacing.y*2.0f) - _style.FramePadding.y );
    ImVec2 texSize = ImGui::CalcTextSize(label);
    dl->AddRectFilled(TexPos,ImVec2(TexPos.x + texSize.x, TexPos.y + texSize.y), ImGui::ColorConvertFloat4ToU32(_style.Colors[ImGuiCol_FrameBg]),1.0f);
    dl->AddText(main_font,FONT_GLOBAL_SIZE,TexPos,ImColor(0.0f,0.0f,0.0f,1.0f),label);
    //ImGui::SetCursorPosY(cpos.y);
}

namespace TS_GUI{
    int EZ_TEXT_INPUT(const char* name,std::vector<char>& buff,int flage =0, BitmapData* bitmap_button = nullptr){
        int state = 0; // 1 buttom pressed, 2 input field updated
        ImGui::SetCursorPosX(_style.FramePadding.x);
        float xpos = ImGui::GetWindowSize().x/2.0f;
        ImGui::TextUnformatted(name);     
        ImGui::SameLine(xpos); 

        if(bitmap_button)
            ImGui::SetNextItemWidth(xpos - (cal_logo_size(*bitmap_button).x + _style.ItemSpacing.x * 2.0f)  - (_style.FramePadding.x + _style.ItemSpacing.x)/2.0f);  
        else ImGui::SetNextItemWidth(xpos - (_style.FramePadding.x + _style.ItemSpacing.x)/2.0f);

        ImGui::PushID(name);
        if(ImGui::InputText("", &buff[0],buff.capacity(),flage))
            state |= 2;
        
        if(bitmap_button){
            ImGui::SameLine();
            if(ImGui::ImageButton((ImTextureID)(intptr_t)bitmap_button->gpu_id,cal_logo_size(*bitmap_button) ))
                state |= 1;
            
        }
        ImGui::PopID();
        return state;
    }
    bool CheckBox(const char* name, bool* val) {

        ImGui::TextUnformatted(name); 			
        ImGui::SameLine(ImGui::GetWindowSize().x / 2.0f);		

        ImVec2 avR = ImGui::GetContentRegionAvail();
        ImVec2 curPos = ImGui::GetCursorScreenPos();
        ImColor co(_style.Colors[ImGuiCol_HeaderHovered] );
        
        float x_size = curPos.x + avR.x - _style.FramePadding.x / 2.0f;
        float y_size = curPos.y  + FONT_GLOBAL_SIZE + _style.ItemInnerSpacing.y/2.0f +_style.ItemSpacing.y;
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(curPos.x + 25.0f,curPos.y), ImVec2(x_size, y_size ) ,*val ? ImU32(co) : IM_COL32(255, 255, 255, 200)); 			
        ImGui::PushID(name);  													
        bool res = ImGui::Checkbox("ON", val);											
        ImGui::PopID();
        return res;
    }
    bool Combo(const char* label, int* current_item, const char* const items[], int items_count) {

        ImGui::TextUnformatted(label); 			
        ImGui::SameLine(ImGui::GetWindowSize().x / 2.0f);		
        ImVec2 avR = ImGui::GetContentRegionAvail();
        ImGui::SetNextItemWidth(avR.x - _style.FramePadding.x / 2.0f);
        ImGui::PushID(label);  											
        bool res = ImGui::Combo("",current_item,items, items_count);	
        ImGui::PopID();
        return res;
    }
}


void On_newLoad(){
    Clearbuffers();
    auto sf = GetSelectedFilesNames();
    strcpy(VREF(folder_dist_buff),  VREF(folder_buff) );
    if(sf.size()==1) { strcpy(VREF(name_buff),sf.at(0).c_str() ); CutStrToDot(VREF(name_buff));}
    else { strcpy(VREF(name_buff), CutStrToFile ( VREF(folder_buff) ) );}
    pbd = ProgBarsDep();
    StopWatch::Reset();
    LEACH::Reset();
    LEACH::Pause   = false;
    LEACH::cancele = false; 
    ts_context = TS_Context();
}

void options_frame(){
    if(ts_context.popup_name[0] == 'E'){
        //ImGui::TextUnformatted("Compression");
        static const char* choices[] = {"None","Fast","Normal","Best"};  
        TS_GUI::Combo("Compression",&ts_context.comp_item,choices,IM_ARRAYSIZE(choices));
        //if dot doesn't exist force default, extension 
        if(TS_GUI::EZ_TEXT_INPUT("File extension",file_extension)==2){ if(!strchr(VREF(file_extension),'.')){strcpy(VREF(file_extension),".tsh");} }
        TS_GUI::CheckBox("Delete on done",&ts_context.Del_af_Dec);
    }
    if(ts_context.popup_name[0] == 'D'){
        TS_GUI::CheckBox("Delete on done",&ts_context.Del_af_Dec);
        TS_GUI::CheckBox("Overwrite file",&ts_context.overwrite_file);
    }
};

void files_frame_1(){
    on_selected([](int i){
        ImVec2 posc = ImGui::GetCursorScreenPos();
        auto& selected_file_name = read_dir.GetFileInfos().at(i);// weak file names    
        auto&& icon = selected_file_name.get_file_icon();
        ImRect rec(posc,ImVec2(posc.x + 200.0f,posc.y + FONT_GLOBAL_SIZE));
        if (rec.Contains(ImGui::GetMousePos())){
            if(ImGui::ImageButton((ImTextureID)(intptr_t)X_icon.gpu_id, ImVec2(X_icon.width, X_icon.hight))){
                selected_file_name.Selection ^=1;
            }
            ImGui::SameLine();
        }
        ImGui::Image((ImTextureID)(intptr_t)icon->gpu_id, ImVec2(icon->width, icon->hight));
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() -0.20f * FONT_GLOBAL_SIZE);
        ImGui::TextUnformatted (selected_file_name.display_name().c_str()); 
        ImGui::Spacing();
    });        
};


void files_frame_2(){
    ImGui::PushStyleColor(ImGuiCol_FrameBg, _style.Colors[ImGuiCol_WindowBg]);
    char buf[32];
    float val = FileProcessingInfo::State().empty() ? 0.0f : FileProcessingInfo::State().back().prce/100.0f;
    sprintf(buf, "%0.1f/%d", val * 100.0f, 100);
    ImGui::ProgressBar(val, ImVec2(ImGui::GetContentRegionAvail().x, 0.f),buf);
    ImGui::TextUnformatted("Elapsed Time:");
    ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - FONT_GLOBAL_SIZE * 2.2f);
    ImGui::TextUnformatted(StopWatch::Get_Strtime());
    ImGui::PopStyleColor(1);

    static InProcInfo ipi[20];
    if(ts_context.elasproc_stat_size != FileProcessingInfo::State().size()){
        FileProcessingInfo::Peak(ipi,IM_ARRAYSIZE(ipi));
        ts_context.elasproc_stat_size = FileProcessingInfo::State().size();
    }
    if(val != 1.0f && val >= 0.0f)
        StopWatch::Counting();
    //prvs=curs;
    ImGui::BeginChild("simple log");
    for (size_t i = 0; i < IM_ARRAYSIZE(ipi); i++)
    {
        if(!ipi[i].stat.empty()){
            ImGui::SetCursorPosX(ImGui::GetCursorPosX()+ 0.5f * FONT_GLOBAL_SIZE);
            if(ipi[i].prce == 100.0f){
                ImGui::TextColored(ImColor(0.0f,0.55f,0.0f).Value,"%s",ipi[i].stat.c_str());
                if(ts_context.Del_af_Dec){
                    on_selected([](int i){ read_dir.GetFileInfos().at(i).DeletetoRecycle(); });
                    OnNewPath();
                    ts_context.Del_af_Dec=false;
                }
            }
            else if(ipi[i].prce < 0.0f){
                ImGui::TextColored(ImColor(0.65f,0.0f,0.0f).Value,"%s",ipi[i].stat.c_str());
            }
            else
                ImGui::TextUnformatted(ipi[i].stat.c_str());
        }
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
}

void elasproc_progs_frame(){
    ImGui::PushStyleColor(ImGuiCol_FrameBg, _style.Colors[ImGuiCol_WindowBg]);
    char buf[32];
    float val = (LEACH::Get_Task_prog() * (100.0 / LEACH::Get_TotalProg_prog()))/100.0f;
    sprintf(buf, "%0.1f/%d", val * 100.0f, 100);
    ImGui::ProgressBar(val, ImVec2(ImGui::GetContentRegionAvail().x, 0.f),buf);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    pbd.curPr = LEACH::Get_Task_prog();
    pbd.delta = (pbd.curPr - pbd.prevPr)*Byte_To_MB;
    pbd.MaxPeak = pbd.delta > pbd.MaxPeak ? pbd.delta : pbd.MaxPeak;
    pbd.prevPr = LEACH::Get_Task_prog();
    bool elaproc_working = Data_ProcessingJob.is_working() && !LEACH::Pause;
    if (!pbd.animate || pbd.refresh_time == 0.0)
        pbd.refresh_time = ImGui::GetTime();
    while (elaproc_working && pbd.refresh_time < ImGui::GetTime()) // Create data at fixed 60 Hz rate for the demo
    {
        ts_context.bars[pbd.values_offset] = pbd.delta;
        pbd.values_offset = (pbd.values_offset + 1) % IM_ARRAYSIZE(ts_context.bars);
        pbd.refresh_time += 1.0f / 30.0f;
    }

    {
        char overlay[32];
        if(elaproc_working){
            float average = 0.0f;
            for (int n = 0; n < IM_ARRAYSIZE(ts_context.bars); n++)
                average += ts_context.bars[n]/ _io.DeltaTime;
            average /= (float)IM_ARRAYSIZE(ts_context.bars);
            sprintf(overlay, "avg.%0.2fMB/s", average );
        }
        ImGui::PlotHistogram("Lines", ts_context.bars, IM_ARRAYSIZE(ts_context.bars), pbd.values_offset, elaproc_working ? overlay:"", 0.0f, pbd.MaxPeak, ImVec2(0, 80.0f));
    }
    
    ImGui::PopStyleColor(1);
};

bool Texted_buttom(const char* txt,const BitmapData& bitmap, bool is_same_line = false){
    if(is_same_line) ImGui::SameLine();
    auto* dl = ImGui::GetWindowDrawList(); 
    ImVec2 bt_pos = ImGui::GetCursorPos();
    ImVec2 bt_sz = ImVec2(bitmap.width,bitmap.hight) * (FONT_GLOBAL_SIZE / 35.0f);
    bool res = ImGui::ImageButton((ImTextureID)(intptr_t)bitmap.gpu_id, bt_sz);
    ImVec2 tx_S = ImGui::CalcTextSize(txt);
    ImVec2 sz = ImVec2(X_midpos_To_Width(bt_pos.x,bt_sz.x,tx_S.x), bt_pos.y + (bt_sz.y * 0.82f));
    dl->AddText(main_font,FONT_GLOBAL_SIZE,sz,ImColor(0.0f,0.0f,0.0f,1.0f),txt);
    return res;
}

void MainOptionsSection(){
    float mo_sizes = FONT_GLOBAL_SIZE / 35.0f;
    if(Texted_buttom("Encrypt",encrypt_icon)){
        if(!Data_ProcessingJob.is_working()){
            ImGui::OpenPopup("###popup");
            On_newLoad();
            ts_context.popup_name = "Encryption";
        }
    }

    if(Texted_buttom("Decrypt",decrypt_icon,true)){
        ImGui::OpenPopup("###popup");
        On_newLoad();
        ts_context.popup_name = "Decryption";
    }

    if(Texted_buttom("Delete",trash_icon,true)){
        ImGui::OpenPopup("Delete");
    }

    auto wz = ImGui::GetWindowSize();
    ImGui::SetNextWindowSize(ImVec2(wz.x/1.5f,wz.y/1.7f));
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    static char buf[50];
    sprintf(buf, "%s###popup",ts_context.popup_name);

    if (ImGui::BeginPopupModal(buf, NULL, POPUP_FLAGS))
    {
        ImGui::PushFont(small_font);
        ImVec2 avon2 = ImGui::GetContentRegionAvail();
        float y_size4_top = FONT_GLOBAL_SIZE*3.0f * (_style.ItemSpacing.y/2.0f);
        if(!ts_context.is_elasprocessing){
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 2.0f);
            ImGui::BeginChild("c1",ImVec2(avon2.x,y_size4_top),true);
            ImGui::Spacing();
            TS_GUI::EZ_TEXT_INPUT("Password",password_buff,ImGuiInputTextFlags_Password);
            int des_feild_res = TS_GUI::EZ_TEXT_INPUT("Destination",folder_dist_buff,0,&fileD_icon);
            TS_GUI::EZ_TEXT_INPUT(ts_context.popup_name[0] == 'E' ? "Name" : "Folder" ,name_buff);

            if(des_feild_res & 1) // input field button was pressed
            {
                const M_CHAR* open_folder_dialog_res = FOLDER_DIALOG;
                if(open_folder_dialog_res){ 
                    M_strcpy(&w_folder_dist_buff[0], open_folder_dialog_res);
                    Dynamic_strcpy(folder_dist_buff,&w_folder_dist_buff[0]);
                }
            }
            if(des_feild_res & 2) // input field was edited
            {
                Dynamic_strcpy(w_folder_dist_buff, &folder_dist_buff[0]);
            }
            ImGui::PopStyleVar();
            ImGui::EndChild();
        }else{
            FramedDataRegion("Files Progress",1,ImVec2( 0.0f,y_size4_top*1.2f),files_frame_2);
            FramedDataRegion("Compression Progress",2,ImVec2( 0.0f,y_size4_top*1.2f),elasproc_progs_frame);
        }

        float x_size = (avon2.x/2.0f) - _style.FramePadding.x;
        float y_size = 150.0f;
        float items_number = 1.0f;
    
        if(!ts_context.is_elasprocessing){
            FramedDataRegion("Options",1,ImVec2( ImGui::GetWindowContentRegionWidth() * 0.5f ,y_size),options_frame);
            ImGui::SameLine();
            FramedDataRegion("Files",2,ImVec2( 0 ,y_size),files_frame_1,true);
        }

        ImGui::SetCursorPosY(avon2.y);
        ImGui::Separator(); ImGui::Spacing();ImGui::Spacing();
        //!ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        if (ImGui::Button( ts_context.is_elasprocessing ? (LEACH::Pause ? "Resume":"Pause") : "OK", ImVec2(120, 0))) { 
            auto sf = GetSelectedFilesNames(); 
            if(ts_context.is_elasprocessing) {
                LEACH::Pause ^= 1;pbd.refresh_time=0;
                goto endif;
            }
            if(sf.empty())  goto endif;
            {
                StopWatch::Start();
                ts_context.is_elasprocessing = true;
                std::string fn(VREF(name_buff));
                std::string fd(VREF(folder_buff)); 
                std::string fdb(VREF(folder_dist_buff)); 
                if(ts_context.popup_name[0] == 'E'){
                
                    std::string ext(VREF(file_extension)); 
                    int comLev = 0;
                    switch (ts_context.comp_item)
                    {
                        case 0: comLev = (int)Compression_levels::NO_COMPRESSION; break;
                        case 1: comLev = (int)Compression_levels::BEST_SPEED; break;
                        case 2: comLev = (int)Compression_levels::DEFAULT_COMPRESSION; break;
                        case 3: comLev = (int)Compression_levels::BEST_COMPRESSION; break;
                    }
                    if(!fn.empty() && !fdb.empty() && sf.size() && Is_path_valid(fdb.c_str()) ){
                        if(sf.size() == 1){
                            Data_ProcessingJob.give(DynamicCompress,fd.append("\\").append(sf.at(0)), fdb.append("\\").append(fn).append(ext),VREF(password_buff),comLev);
                        }
                        else  
                        {
                            fd.append("\\");
                            for(auto& i: sf) 
                                i.insert(0,fd.c_str());
                            Data_ProcessingJob.give(CompressCollection, sf, fn, fdb.append("\\").append(fn).append(ext),VREF(password_buff)); 
                        }
                    }else{
                        Error_stat("Empty field or uncorrect distenation path")
                    }
                }

                if(ts_context.popup_name[0] == 'D'){
                    if(!fd.empty() && !fdb.empty()){   
                        fdb.append("\\").append(fn);
                        std::string src = fd.append("\\").append(sf.at(0));

                        if(read_dir.find(CutStrToFile(fdb.c_str())) && !ts_context.overwrite_file){
                            fdb.append("_copy");
                        }

                        Data_ProcessingJob.give(DynamicDecompress,src, fdb,VREF(password_buff));     
                    }
                }
            }
            endif:;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button(Data_ProcessingJob.is_working() && LEACH::cancele ? "Canceling..":"Cancel", ImVec2(120, 0))) { 
            LEACH::cancele = true;
            if(!Data_ProcessingJob.is_working()){
                ts_context.is_elasprocessing = false; 
                FileProcessingInfo::Clear();
                ImGui::CloseCurrentPopup(); 
            }
            OnNewPath();
        }
        // Once canceling is ready, do close everything then cancel
        if(LEACH::cancele && !Data_ProcessingJob.is_working() && ts_context.is_elasprocessing){
            ts_context.is_elasprocessing = false; 
            FileProcessingInfo::Clear();
            ImGui::CloseCurrentPopup(); 
        }
        ImGui::SameLine();

        ImGui::PopFont();
        ImGui::EndPopup();
    }

    //ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Delete", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        //std::vector<FileInfo>
        ImGui::SetItemDefaultFocus();
        static bool er_oc = false;
        ImGui::PushFont(small_font);
        //std::for_each()
        if(!er_oc){
            ImGui::Text("These files will be movied to Recycle Bin.\nThis operation cannot be undone!\n");
        }else{
            ImGui::Text("Coudn't move these files to Recycle Bin:  \n");
        }
        ImGui::Spacing();
        on_selected([](int i){ImGui::BulletText("%s",read_dir.GetFileInfos().at(i).display_name().c_str());});
        ImGui::Separator();
        ImGui::PopFont();
        if(!er_oc){
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                on_selected([](int i) { 
                    auto& infof = read_dir.GetFileInfos().at(i);  
                    if (infof.DeletetoRecycle()) { er_oc = true; }
                    else { infof.Selection ^= 1; }; 
                 });
                if(!er_oc){
                    OnNewPath();
                    ImGui::CloseCurrentPopup();
                }  
            }
            ImGui::SameLine();
        }
        if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup();OnNewPath();er_oc=false;}
        if(er_oc) {ImGui::SameLine();ImGui::Dummy(ImVec2(120, 0));}
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Move", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }
}


float Repeat(float t, float length)
{
    return ImClamp(t - ImFloor(t / length) * length, 0.0f, length);
}


void FileBrowserSection(){
    if(ImGui::ImageButton((ImTextureID)(intptr_t)to_up_icon.gpu_id,cal_logo_size(to_up_icon)))
    {
        CutStrToPath( VREF(folder_buff));
        Dynamic_strcpy(w_folder_buff, VREF(folder_buff));
        OnNewPath();
    } 
    ImGui::SameLine();
    
    if(ImGui::ImageButton((ImTextureID)(intptr_t)fileD_icon.gpu_id,cal_logo_size(fileD_icon) )){
        const M_CHAR* open_folder_dialog_res = FOLDER_DIALOG;

        if(open_folder_dialog_res){ 
            RESV_B_STRD(w_folder_buff,open_folder_dialog_res);
            folder_buff.resize(w_folder_buff.capacity());

            M_strcpy( VREF(w_folder_buff), open_folder_dialog_res);
            OnNewPath();
        }
    } 

    float getAvSpace = to_up_icon.width + fileD_icon.width + enter_icon.width ;
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - cal_logo_size(enter_icon).x - _style.ItemSpacing.x*2.0f);

        //!============================
    //std::string folderPathstr = std::string(&buf[0]);
    ImGui::InputText("", VREF(folder_buff),folder_buff.capacity());
    ImGui::SameLine();
    //printf("%i\n",strlen (&gui_folder_path_buff[0]));

    if(ImGui::ImageButton((ImTextureID)(intptr_t)enter_icon.gpu_id,cal_logo_size(enter_icon))){
        Dynamic_strcpy(w_folder_buff, VREF(folder_buff) );
        OnNewPath();
    }
        
    List();  
}

void ProcessDroppedFiles(App_Window* window){
    auto d_data = window->Get_dropped_files(); 
    if(d_data){
        std::string t_path = d_data->at(0);
        CutStrToPath(VREF(t_path) );
        Dynamic_strcpy(w_folder_buff, VREF(t_path) );
        OnNewPath(); 
        for(int i = 0; i < d_data->size(); i++){
            std::string tempS = CutStrToFile(&d_data->at(i)[0]);
            auto& infof = read_dir.GetFileInfos();
            for(int i2 = 0; i2 < infof.size(); i2++){
                if (infof.at(i2).display_name() == tempS){
                    infof.at(i2).Selection ^=  1;
                }
            }           
        }
        d_data->clear();
    }
}

void ProcessDoubleClickSelction(){
    if(ImGui::IsPopupOpen("###popup")) return;
    
    on_selected([](int i){
        if(ImGui::IsMouseDoubleClicked(0) && read_dir.GetFileInfos().at(i).get_type() == "Directory"){
            int ss=ts_context.clickedPos.y - ImGui::GetMousePos().y;
            if(ss < 0 || ss > 30 ) return; //outside the bound of selectable
            strcat(VREF(folder_buff),"\\");
            strcat(VREF(folder_buff),read_dir.GetFileInfos().at(i).display_name().c_str());
            Dynamic_strcpy(w_folder_buff, VREF(folder_buff) );
            OnNewPath();
        }
    });
}

namespace AppMain{
    // Runs once before the creation of the window
    void AppAwake(App_Window* window){
        glfwWindowHint(GLFW_DECORATED, false);
        w_folder_buff.resize(300);
        folder_buff.resize(300);
        w_folder_dist_buff.resize(300);
        folder_dist_buff.resize(300);
        name_buff.resize(MAX_FILE_NAME);
        password_buff.resize(10);
        file_extension.resize(10); strcpy(VREF(file_extension),".tsh");
    }

    void AppStart(App_Window* window){

        main_font   = LoadFont( sizeof(BP_segoeui),BP_segoeui,FONT_SIZE);
        small_font  = LoadFont( sizeof(BP_segoeui),BP_segoeui,FONT_SIZE/1.2f);
        MakeLogoBitmaps(to_up_icon,BP_segmdl2,TOUP_INDEX, LOGO_SIZE);
        MakeLogoBitmaps(fileD_icon,BP_segmdl2,FILED_INDEX,LOGO_SIZE);
        MakeLogoBitmaps(enter_icon,BP_segmdl2,ENTER_INDEX, LOGO_SIZE,16,16);
        MakeLogoBitmaps(X_icon,BP_segmdl2,X_INDEX, 15,0,0,0,0);

        const int mo_sizes = 120.0f;
        MakeLogoBitmaps(encrypt_icon,BP_segmdl2,ENC_INDEX, mo_sizes,6,40,0,19);
        MakeLogoBitmaps(decrypt_icon,BP_segmdl2,DEC_INDEX, mo_sizes,6,40,0,19);

        // In order for the width of the bitmab to be equal to the previous two, we first gets the box size through GetLogoBitmapsSize
        // then we subtract the width of base bitmap with taget bitmap. 
        static int width,height;
        GetLogoBitmapsSize(trash_icon,BP_segmdl2,TRASH_INDEX,mo_sizes,&width,&height);
        MakeLogoBitmaps(trash_icon,BP_segmdl2,TRASH_INDEX, mo_sizes,encrypt_icon.width-width,40,0,19);

        ImGui::GetStyle() = MyGuiStyle();       
    }
    // Runs everyfram
    void AppUpdate(App_Window* window, float deltaTime){
        ProcessDroppedFiles(window);
        MainOptionsSection();
        FileBrowserSection();  
        ProcessDoubleClickSelction();
        //ImGui::ShowDemoWindow();
    }
    // Runs once when app is closed
    void AppClosed(App_Window* window){
        //DEBUG_LOG(To_temp_path());
        RemoveDir(To_temp_path());
    }
}