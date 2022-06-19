//#define LEACH_IMPL

#ifndef LEACH_H
#define LEACH_H

namespace LEACH{
    const unsigned int Get_TotalProg_prog();
    const unsigned int Get_Task_prog();
    const bool Is_Progress();
    void Reset();
    extern volatile bool Pause;
    extern bool cancele;
    
} 
#endif

#ifdef LEACH_IMPL
namespace LEACH{
    unsigned int Total_prog = 0;
    unsigned int Task_prog = 0;
    bool Progress = true;
    volatile bool Pause = false;
    bool cancele = false;
    const unsigned int Get_TotalProg_prog(){return Total_prog;}
    const unsigned int Get_Task_prog(){return Task_prog;}
    const bool Is_Progress(){return Progress;}
    void Reset(){Total_prog=0; Task_prog=0; Progress=true; }
} 
#endif