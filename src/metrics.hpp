#pragma once
struct Frame { double end,ms; };
struct Summary { unsigned n; double fps,p99; };
inline void sift(double* a,unsigned len,unsigned root){
    for(unsigned child=root*2+1;child<len;child=root*2+1){
        if(child+1<len && a[child]<a[child+1]) ++child;
        if(a[root]>=a[child]) break;
        double tmp=a[root];a[root]=a[child];a[child]=tmp;root=child;
    }
}
inline Summary summarize(const Frame* frames,unsigned n,double start,double finish,double* scratch){
    unsigned count=0;double total=0;
    for(unsigned i=0;i<n;++i){auto f=frames[i];
        if(f.ms>0 && f.end<=finish && f.end-f.ms*.001>=start-1e-8){
            scratch[count++]=f.ms;total+=f.ms;
        }
    }
    if(!count)return {};
    for(unsigned i=count/2;i;i--)sift(scratch,count,i-1);
    for(unsigned i=count-1;i;i--){double t=scratch[0];scratch[0]=scratch[i];scratch[i]=t;sift(scratch,i,0);}
    return {count,1000*count/total,scratch[(99*count+99)/100-1]};
}
inline double live(const Frame* frames,unsigned n,double now){
    if(!n || now<frames[n-1].end || now-frames[n-1].end>.30)return 0;
    double sum=0;unsigned count=0;
    for(unsigned i=n;i && frames[n-1].end-frames[i-1].end<=.10;--i){sum+=frames[i-1].ms;++count;}
    return sum>0?1000*count/sum:0;
}

// Measure the union of observed presentation intervals inside the recording.
// This detects a stream that appeared only near the end and avoids counting overlap twice.
inline bool coversSession(const Frame* frames,unsigned n,double start,double finish){
    if(!(finish>start))return false;
    double through=start,covered=0;
    for(unsigned i=0;i<n;++i){
        const auto f=frames[i];
        if(f.end<=start)continue;
        double first=f.end-f.ms*.001;
        if(first>=finish)continue;
        double last=f.end<finish?f.end:finish;
        if(first<through)first=through;
        if(last>first){covered+=last-first;through=last;}
    }
    return covered>=(finish-start)*.90;
}
