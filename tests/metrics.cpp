#include <initializer_list>
#include "../src/metrics.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
 Frame rows[300];unsigned n=0;double t=10,scratch[300];
 for(int rate: {60,120,143}){for(int i=0;i<40;++i){t+=1./rate;rows[n++]={t,1000./rate};}
 assert(std::abs(live(rows,n,t)-rate)<1e-7);}
 assert(live(rows,n,t+.31)==0);
 Frame irregular[]={{.01,10},{.04,30},{.05,0}};
 auto s=summarize(irregular,3,0,1,scratch);assert(s.n==2&&s.fps==50&&s.p99==30);
 assert(summarize(irregular,3,.011,1,scratch).n==0);
 assert(summarize(irregular,3,.01,1,scratch).n==1);
 puts("C++: 60/120/143 FPS, stale data, weighted FPS, P99 and interval boundaries passed.");
}
