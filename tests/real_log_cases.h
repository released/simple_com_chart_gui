#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct RealLogCase {
    const char* file;
    const char* input;
    std::unordered_map<std::string,double> expected;
    const char* include;
    const char* exclude;
    bool accepted;
};
inline const std::vector<RealLogCase>& real_log_cases() {
    // Expected maps are manually reviewed observations, never parser-generated output.
    static const std::vector<RealLogCase> cases{
        {"20261028_140028_1.log","[14:00:33.398] [RX] [FORCE_HAPTIC][P3:76.1803][STATE:HOLD]",{{"P3",76.1803}},"[FORCE_HAPTIC]","STOP",true},
        {"teraterm2.log","[DW7912][INIT][OK][ADDR:0x59][WAVE:OLED_Square][SIZE:150]",{{"ADDR",89},{"SIZE",150}},"[DW7912]","ERR",true},
        {"teraterm3.log","[S07],raw:,-5264,avg16:,-5261,before:,-4669,delta:,-592,[S08],raw:,5247,avg16:,5247,before:,6030,delta:,-783,[S13],raw:,-18784,avg16:,-18791,before:,-18627,delta:,-164,[S15],raw:,-5521,avg16:,-5514,before:,-5225,delta:,-289,[formula],(S7:,-592),(S8:,-783),(S13:,-164),(S15:,-289),[Z]:,-148.9309,[force_p3]:,-37.9774",
            {{"[S07].raw",-5264},{"[S07].avg16",-5261},{"[S07].before",-4669},{"[S07].delta",-592},
             {"[S08].raw",5247},{"[S08].avg16",5247},{"[S08].before",6030},{"[S08].delta",-783},
             {"[S13].raw",-18784},{"[S13].avg16",-18791},{"[S13].before",-18627},{"[S13].delta",-164},
             {"[S15].raw",-5521},{"[S15].avg16",-5514},{"[S15].before",-5225},{"[S15].delta",-289},
             {"[formula].S7",-592},{"[formula].S8",-783},{"[formula].S13",-164},{"[formula].S15",-289},{"[Z]",-148.9309},{"[force_p3]",-37.9774}},"[S07]","ERR",true},
        {"teraterm11.log","-3.6767",{{"value",-3.6767}},"","",true},
        {"teraterm22.log","IIC11 MD_STATUS:0x02",{{"IIC11 MD_STATUS",2}},"IIC11","busy",true},
        {"teraterm33.log","[G1:0][0x50][NG][MCFG:0x00,exp:0xD4]",{{"G1",0},{"MCFG",0},{"exp",212}},"[G1","[NG]",false},
        {"teraterm44.log","[DW7912][INIT][OK][ADDR:0x59][WAVE:OLED_Square][SIZE:150]",{{"ADDR",89},{"SIZE",150}},"[DW7912]","INIT",false},
        {"20261028_104821_1.log","[10:48:27.276] [RX] 0.5110",{{"value",.511}},"[RX]","ERR",true}
    };
    return cases;
}
