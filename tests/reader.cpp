// Exercise the production CSV decoder with both supported PresentMon layouts.
#define entry circe_windows_entry
#include "../src/app.cpp"
static void feed(const char* input){unsigned n=0;while(input[n]){line[n]=input[n];++n;}line[n]=0;row();}
int main(){
 for(int layout=0;layout<2;++layout){
    ciPid=ciTime=ciChain=ciMs=-1;pid=123;freq=10000000;count=0;lastFrame=0;mainChain=0;
    feed(layout?"Application,ProcessID,SwapChainAddress,MsBetweenPresents,TimeInQPC":
                "Application,ProcessID,SwapChainAddress,msBetweenPresents,QPCTime");
    feed("game.exe,124,0x01,10,9000000");
    feed("game.exe,123,0x01,10,10100000");
    feed("game.exe,123,0x01,10,10200000");
    if(count!=2 || live(frames,count,1.025)!=100)return 1;
    count=0;mainChain=0;lastFrame=0;
    feed("game.exe,123,NA,10,10100000");
    feed("game.exe,123,0x0000000000000000,10,10100000");
    if(count!=1 || live(frames,count,1.015)!=100)return 2;
    resetReader();
    const char csv[]="Application,ProcessID,SwapChainAddress,MsBetweenPresents,TimeInQPC\r\n"
                      "\"game, wide.exe\", \"123\", \"0x0123\", \"8.0\", \"12000000\"\r\n";
    for(unsigned j=0;j<sizeof(csv)-1;++j)consumeBytes(csv+j,1);
    if(count!=1||mainChain!=0x123||live(frames,count,lastFrame)!=125)return 3;
    feed("game,123,0x1,1X,12200000");feed("game,123x,0x1,10,12200000");
    feed("game,123,0x1,10,184467440737095516160");
    if(count!=1||counters.invalid!=3)return 4;
    resetReader();
 }
}
