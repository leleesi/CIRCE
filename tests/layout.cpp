#include "../src/layout.hpp"
#include <cassert>
#include <cstdio>
using namespace panelui;
static bool overlaps(Box a,Box b){
    return a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h;
}
int main(){
    bool seen[Total]{};
    for(const auto& a:items){
        assert(!seen[a.id]);seen[a.id]=true;
        assert(a.box.x>=0&&a.box.y>=0&&a.box.w>0&&a.box.h>0);
        assert(a.box.x+a.box.w<=pageW&&a.box.y+a.box.h<=pageH);
        for(const auto& b:items){
            if(&a==&b|| (a.page!=b.page&&a.page!=Common&&b.page!=Common))continue;
            assert(!overlaps(a.box,b.box));
        }
    }
    puts("Painel: controlos das três páginas dentro dos limites, sem sobreposição.");
}
