#define entry circe_windows_entry
#include "../src/app.cpp"
#include <cstdio>
#include <cassert>
static void consume(const char* text){unsigned i=0;while(text[i]&&text[i]!='\r'&&text[i]!='\n'){line[i]=text[i];++i;}line[i]=0;row();}
int main(int argc,char** argv){
    assert(argc==2);FILE* file=fopen(argv[1],"rb");assert(file);
    pid=24560;freq=10000000;char input[8192];
    while(fgets(input,sizeof(input),file))consume(input);
    fclose(file);printf("CSV de referência: %llu linhas, %llu intervalos aceites para Presenter.exe, %llu linhas de outros processos, %.2f FPS.\n",counters.lines,counters.accepted,counters.wrongPid,live(frames,count,lastFrame));
    assert(count==8 && counters.accepted==8 && counters.wrongPid==48);
    assert(live(frames,count,lastFrame)>105 && live(frames,count,lastFrame)<109);
    count=0;lastFrame=0;mainChain=0;surfaceEpoch=0;for(auto& rate:surfaces)rate={};
    for(int i=0;i<150;++i){
        if(i%30==0){snprintf(input,sizeof(input),"game.exe,24560,0x1,200,%d",10000000+i*70000); // compact schema
            if(!i)consume("Application,ProcessID,SwapChainAddress,msBetweenPresents,QPCTime");consume(input);}
        snprintf(input,sizeof(input),"game.exe,24560,0x2,7,%d",10010000+i*70000);consume(input);
    }
    assert(mainChain==2&&live(frames,count,lastFrame)>142);
    printf("Duas superfícies ativas: superfície do jogo selecionada, %.2f FPS.\n",live(frames,count,lastFrame));
}
