#include "../src/win.hpp"
#include "../src/text.hpp"
static H game=(H)12,lastGameWindow=nullptr;static U pid=22,stage=0;static int selection=-1,added=0;static H attached=nullptr;
static void say(const W*){}
static bool attach(H h){attached=h;return true;}
#include "../src/game_list.hpp"
API I IsWindow(H){return 1;}
API I IsWindowVisible(H){return 1;}
API H GetShellWindow(){return (H)1;}
API U GetCurrentProcessId(){return 99;}
API U GetWindowThreadProcessId(H h,U* p){*p=h==(H)11?21:h==(H)12?22:99;return 1;}
API I GetWindowTextW(H,W* s,I){s[0]=L'G';s[1]=0;return 1;}
API H OpenProcess(U,I,U){return nullptr;}
API I CloseHandle(H){return 1;}
API I QueryFullProcessImageNameW(H,U,W*,U*){return 0;}
API I EnumWindows(I(*visit)(H,L),L p){for(Q i=10;i<=12;++i)visit((H)i,p);return 1;}
API L SendMessageW(H,U m,Q w,L){if(m==0x14b){added=0;selection=-1;}if(m==0x143)return added++;if(m==0x14e)selection=(int)w;return selection;}
int main(){refreshGames();if(added!=3||selection!=0)return 1;selection=1;chooseGame();if(attached!=(H)11||!manualTarget)return 2;
attached=nullptr;choices[1].pid=300;chooseGame();if(attached)return 3;selection=0;chooseGame();return manualTarget;}
