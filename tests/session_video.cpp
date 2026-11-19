// Runs the production session writer, renderer and PDF generator. POSIX substitutes
// only file/process handles. The two decoders and the encoder are real FFmpeg processes.
#include "../src/app.cpp"
#include <cassert>
#include <cerrno>
#include <chrono>
#include <codecvt>
#include <locale>
#include <set>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
namespace fs=std::filesystem;
unsigned platform::lastFlushError(){return 0;}
static bool failCommit;
static bool ffmpegAvailable=true;
static std::map<pid_t,int> finished;
constexpr uintptr_t processBit=1ULL<<40;
static H fh(int fd){return fd<0?bad:(H)(uintptr_t(fd)+1);}
static int fd(H h){return int(uintptr_t(h)-1);}
static std::string host(const W* path) {
    std::u16string text;
    for (; *path; ++path) text += *path == L'\\' ? u'/' : char16_t(*path);
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t>{}.to_bytes(text);
}
static void wide(const std::string& text, W* result) {
    auto decoded = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t>{}.from_bytes(text);
    for (char16_t ch : decoded) *result++ = W(ch);
    *result = 0;
}
static void widePath(const std::string& name, Text<2048>& path) {
    path.clear(); wide(name,path.buf); path.len=lstrlenW(path.buf);
    for (unsigned i=0;i<path.len;++i) if (path.buf[i]==L'/') path.buf[i]=L'\\';
}
static platform::Choice dialogResult = platform::Choice::Selected;
static std::string defaultVideos, dialogPath, storedFolder, status, suggestedFile, initialFolder;
static std::set<std::string> allowedFolders;
static bool overwrite, storageWorks=true;
static int pickerCalls, confirmCalls, workerCalls;
void panelui::set(panelui::Id id,const W* value) { if(id==panelui::Status) status=host(value); }
bool panelui::confirm(void*,const W*,const W*,const W*,const W*,bool) { ++confirmCalls; return overwrite; }
bool platform::defaultOutputFolder(W* path,unsigned) {
    if(defaultVideos.empty()) return false;
    Text<2048> dir;widePath(defaultVideos,dir);memcpy(path,dir.buf,(dir.len+1)*sizeof(W));return true;
}
bool platform::readOutputFolder(W* path,unsigned) {
    if(storedFolder.empty()) return false;
    Text<2048> dir;widePath(storedFolder,dir);memcpy(path,dir.buf,(dir.len+1)*sizeof(W));return true;
}
bool platform::storeOutputFolder(const W* path) {
    if(!storageWorks) return false;
    storedFolder=host(path);allowedFolders.insert(storedFolder);return true;
}
bool platform::knownOutputFolder(const W* path) { return allowedFolders.contains(host(path)); }
platform::Choice platform::chooseOutput(void*,OutputKind kind,const W* dir,const W* name,W* path,unsigned capacity) {
    ++pickerCalls;initialFolder=host(dir);suggestedFile=name?host(name):"";
    if(dialogResult!=Choice::Selected) return dialogResult;
    Text<2048> choice;widePath(dialogPath,choice);assert(choice.len<capacity);
    memcpy(path,choice.buf,(choice.len+1)*sizeof(W));return Choice::Selected;
}
API H CreateThread(Security*,Q,U(*start)(void*),void* arg,U,U*) {
    ++workerCalls;start(arg);return (H)1;
}
API I QueryPerformanceCounter(L* ticks) { *ticks=0;return 1; }
API U GetFileAttributesW(const W* path) {
    auto f=fs::status(host(path));
    return fs::is_directory(f)?0x10:fs::is_regular_file(f)?0x80:0xffffffff;
}
API I lstrcmpiW(const W* a,const W* b){
    auto lower=[](W c){return c>=L'A'&&c<=L'Z'?c+32:c;};
    while(*a&&lower(*a)==lower(*b)){++a;++b;}return lower(*a)-lower(*b);
}
API I lstrlenW(const W* w){int n=0;while(w[n])++n;return n;}
API int WideCharToMultiByte(U,U,const W* s,I,char* out,I cap,const char*,I*){
    int n=lstrlenW(s);if(cap<=n)return 0;
    for(int i=0;i<=n;++i)out[i]=s[i]<256?char(s[i]):'?';return n+1;
}
API I GetWindowTextW(H h,W* text,I){const W* s=h==labelA?L"Médio":L"Ultra";int n=lstrlenW(s);memcpy(text,s,(n+1)*sizeof(W));return n;}
API H CreateFileW(const W* name,U access,U, H,U creation,U flags,H){
    std::string path=host(name);if(path=="NUL")path="/dev/null";
    int mode=(access&0x40000000)?O_RDWR:O_RDONLY;
    if(creation==1)mode|=O_CREAT|O_EXCL;if(creation==2)mode|=O_CREAT|O_TRUNC;
    auto f=open(path.c_str(),mode|O_CLOEXEC,0600);
    if(f>=0 && (flags&0x04000000)) unlink(path.c_str());
    return fh(f);
}
API I ReadFile(H h,void* out,U n,U* got,H){ssize_t r=read(fd(h),out,n);*got=r<0?0:r;return r>=0;}
API I WriteFile(H h,const void* in,U n,U* done,H){
    ssize_t r=write(fd(h),in,std::min(n,65536U));*done=r<0?0:r;return r>=0;
}
API I FlushFileBuffers(H h){return fsync(fd(h))==0;}
API I GetFileSizeEx(H h,L* size){
    auto off=lseek(fd(h),0,SEEK_CUR);*size=lseek(fd(h),0,SEEK_END);lseek(fd(h),off,SEEK_SET);return *size>=0;
}
API I CloseHandle(H h){
    if((uintptr_t(h)&processBit)!=0){finished.erase(pid_t(uintptr_t(h)&~processBit));return 1;}
    return h&&close(fd(h))==0;
}
API I SetHandleInformation(H,U,U){return 1;}
API I CreatePipe(H* read,H* write,Security*,U){int ends[2];if(pipe2(ends,O_CLOEXEC))return 0;*read=fh(ends[0]);*write=fh(ends[1]);return 1;}
API I DeleteFileW(const W* path){return unlink(host(path).c_str())==0;}
API I MoveFileExW(const W* a,const W* b,U flags){
    if(failCommit||(!(flags&1)&&fs::exists(host(b))))return 0;
    return rename(host(a).c_str(),host(b).c_str())==0;
}
API void GetSystemTimeAsFileTime(Q* t){static Q clock=12345000;*t=++clock;}
API Q GetTickCount64(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
API U GetCurrentProcessId(){return getpid();}
static bool pollChild(pid_t pid){
    if(finished.contains(pid))return true;
    int status=0;auto got=waitpid(pid,&status,WNOHANG);
    if(got!=pid)return false;
    finished[pid]=WIFEXITED(status)?WEXITSTATUS(status):1;return true;
}
API U WaitForSingleObject(H h,U ms){
    auto pid=pid_t(uintptr_t(h)&~processBit);auto end=GetTickCount64()+ms;
    while(!pollChild(pid)){
        if(GetTickCount64()>=end)return 258;
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }return 0;
}
API I GetExitCodeProcess(H h,U* code){auto pid=pid_t(uintptr_t(h)&~processBit);*code=pollChild(pid)?finished[pid]:259;return 1;}
API I TerminateProcess(H h,U){return kill(pid_t(uintptr_t(h)&~processBit),SIGKILL)==0;}
API I CreateProcessW(const W*,W*,H,H,I,U,H,const W*,Start*,Process*){return 0;}
bool platform::startChild(const W*,W* command,const W*,void* in,void* out,void* err,void** process,void** thread){
    std::vector<std::string> args;std::string token;bool quoted=false;
    for(const char c:host(command)){
        if(c=='"'){quoted=!quoted;continue;}
        if(c==' '&&!quoted){if(!token.empty())args.push_back(token);token.clear();}
        else token+=c;
    }
    if(!token.empty())args.push_back(token);
    assert(!args.empty());
    if(args[0].ends_with("CIRCEJanela.exe")) {
        // Substitute two captured frames. The normal recording command invokes the real FFmpeg encoder.
        pid_t pid=fork();assert(pid>=0);
        if(!pid) {
            unsigned char pixels[16*16*4]{};
            std::ofstream journal(args.back(),std::ios::binary|std::ios::trunc);
            for(double time : {1.0,1.04}) {
                journal.write(reinterpret_cast<const char*>(&time),sizeof(time));
                if(write(fd(out),pixels,sizeof(pixels))!=sizeof(pixels)) _exit(2);
            }
            journal.close();_exit(0);
        }
        *process=(H)(processBit|uintptr_t(pid));*thread=nullptr;return true;
    }
    assert(args[0].ends_with("ffmpeg.exe"));
    if(!ffmpegAvailable)return false;
    args[0]="ffmpeg";std::vector<char*> argv;
    for(auto& arg:args)argv.push_back(arg.data());argv.push_back(nullptr);
    pid_t pid=fork();assert(pid>=0);
    if(!pid){
        dup2(fd(in),0);dup2(fd(out),1);dup2(fd(err),2);
        execvp(argv[0],argv.data());_exit(127);
    }
    *process=(H)(processBit|uintptr_t(pid));*thread=nullptr;return true;
}
static void session(const std::string& dir,int index,int fps,bool commit=true){
    begin=10;finish=25;count=15*fps;
    for(U i=0;i<count;++i)frames[i]={begin+(i+1.)/fps,1000./fps};
    Summary result=summarize(frames,count,begin,finish,scratch);
    assert(result.n==count&&coversSession(frames,count,begin,finish));
    assert(!coversSession(frames+count-3,3,begin,finish));
    widePath(dir+(index?"/clipe-2.mkv":"/clipe-1.mkv"),pending);filming=true;
    assert(save(index,result)==commit);
}
static Saved disk(const Text<2048>& pair){
    Saved result{};std::ifstream file(host(pair.buf),std::ios::binary);
    file.read(reinterpret_cast<char*>(&result),sizeof(result));assert(file);return result;
}
static void checkSnapshot(const std::string& dir) {
    counters.accepted=42;
    counters.bytes=1024;
    std::strcpy(counters.header,"ProcessID,msBetweenPresents");
    assert(writeSnapshot());
    std::ifstream snapshot(dir+"/leitor-fps.txt",std::ios::binary);
    std::u16string text;
    char16_t ch=0;
    while(snapshot.read(reinterpret_cast<char*>(&ch),sizeof(ch)))text.push_back(ch);
    assert(!text.empty() && text[0]==u'\ufeff');
    assert(text.find(language::english()?u"Accepted intervals: 42":u"Intervalos aceites: 42")!=text.npos);
    assert(text.find(language::english()?u"Header: ProcessID,msBetweenPresents":u"Cabeçalho: ProcessID,msBetweenPresents")!=text.npos);
}
int main(int argc,char** argv){
    assert(argc==2);std::string dir=argv[1];widePath(dir,data);
    labelA=(H)10;labelB=(H)11;wide("game.exe",gameExe);
    std::string beforeDir=dir+"/Clipes ação", afterDir=dir+"/Mais clipes",
                finalDir=dir+"/Exportação à escolha", unknownDir=dir+"/Não escolhida";
    for(const auto& path : {beforeDir,afterDir,finalDir,unknownDir}) fs::create_directory(path);
    const std::string pdfDir=finalDir+"/Relatórios", movieDir=finalDir+"/Vídeos finais";
    fs::create_directory(pdfDir);fs::create_directory(movieDir);
    Text<2048> rootPath=parentFolder(L"D:\\partida.mp4"), joined;
    assert(host(rootPath.buf)=="D:/" && joinPath(rootPath,L"jogo.mp4",joined));
    assert(host(joined.buf)=="D:/jogo.mp4");
    auto share=parentFolder(L"\\\\servidor\\partilha\\vídeo.mp4");
    assert(host(share.buf)=="//servidor/partilha" && joinPath(share,L"ação.pdf",joined));
    assert(host(joined.buf)=="//servidor/partilha/ação.pdf");
    Text<2048> full;while(full.len<2047)full.buf[full.len++]=L'x';
    assert(!joinPath(full,L"jogo.mp4",joined));
    loadOutputFolder();assert(host(outputFolder.buf)==dir);
    defaultVideos=dir+"/Vídeos";fs::create_directory(defaultVideos);loadOutputFolder();
    assert(!outputFolderChosen && host(outputFolder.buf)==defaultVideos);
    dialogPath=beforeDir;dialogResult=platform::Choice::Cancelled;assert(!prepareClipFolder());
    assert(storedFolder.empty() && !outputFolderChosen && host(outputFolder.buf)==defaultVideos);
    dialogResult=platform::Choice::Selected;
    dialogPath=dir+"/missing/folder";chooseRecordingFolder();assert(storedFolder.empty());
    storageWorks=false;dialogPath=beforeDir;chooseRecordingFolder();assert(storedFolder.empty());
    storageWorks=true;assert(prepareClipFolder() && storedFolder==beforeDir && outputFolderChosen);
    outputFolder.clear();loadOutputFolder();assert(host(outputFolder.buf)==beforeDir);
    int priorPickers=pickerCalls;assert(prepareClipFolder() && priorPickers==pickerCalls);
    fs::rename(beforeDir,beforeDir+".off");assert(!prepareClipFolder() && storedFolder==beforeDir);
    fs::rename(beforeDir+".off",beforeDir);
    recordW=recordH=16;finish=3;
    assert(recordVideo(false));
    assert(WaitForSingleObject(grab.proc,5000)==0 && WaitForSingleObject(encode.proc,5000)==0);
    assert(successful(grab) && successful(encode));release(grab);release(encode);
    fs::path captured=host(pending.buf);
    assert(captured.parent_path()==beforeDir && fs::file_size(captured)>128);
    assert(fs::file_size(fs::path(captured).replace_extension(".qpc"))==2*sizeof(double));
    pending.clear();
    fs::rename(dir+"/clipe-1.mkv",beforeDir+"/clipe-1.mkv");
    fs::rename(dir+"/clipe-2.mkv",afterDir+"/clipe-2.mkv");
    session(beforeDir,0,100);auto original=pairPath;
    assert(validDocument(saved)&&std::abs(saved.summary[0].fps-100)<.001);
    dialogPath=afterDir;chooseRecordingFolder();assert(storedFolder==afterDir);
    failCommit=true;session(afterDir,1,50,false);failCommit=false;
    assert(!saved.summary[1].n&&!disk(original).summary[1].n);
    session(afterDir,1,50);assert(std::abs(saved.summary[1].fps-50)<.001);
    assert(host(saved.names[0])=="Médio");
    for(int side=0;side<2;++side){
        std::ofstream stream(side? afterDir+"/clipe-2.qpc":beforeDir+"/clipe-1.qpc",std::ios::binary);
        for(int i=0;i<4;++i){const double t=10.5+i*.5+(side?.25:0);stream.write((char*)&t,sizeof(t));}
    }
    std::ofstream(unknownDir+"/clipe-3.mkv").put('x');
    Text<2048> unknown;widePath(unknownDir+"/clipe-3.mkv",unknown);
    resolveSidecar(unknown.buf,original.buf,L"clipe-",L".mkv");assert(!unknown.buf[0]);
    // Same-directory portable comparisons and old clips stored next to the data still resolve.
    fs::path beside=fs::path(dir)/"clipe-3.mkv";std::ofstream(beside).put('x');
    widePath(unknownDir+"/clipe-3.mkv",unknown);resolveSidecar(unknown.buf,original.buf,L"clipe-",L".mkv");
    assert(host(unknown.buf)==beside.string());
    assert(host(original.buf).find("comparacao-")!=std::string::npos);
    for (auto lang : {language::Id::Portuguese, language::Id::English}) {
        language::active=lang;checkSnapshot(dir);saved=disk(original);
        for (int i=0;i<2;++i) {
            resolveSidecar(saved.paths[i],original.buf,L"clipe-",L".mkv");
            resolveSidecar(saved.framePaths[i],original.buf,L"sessao-",L".frames");
            assert(host(saved.paths[i])==(i?afterDir+"/clipe-2.mkv":beforeDir+"/clipe-1.mkv"));
            assert(*saved.framePaths[i]);
        }
        const std::string tag=language::english()?"en":"pt";
        dialogResult=platform::Choice::Cancelled;int workers=workerCalls;
        render();exportPdf();assert(workerCalls==workers);
        dialogResult=platform::Choice::Failed;render();assert(workerCalls==workers);
        assert(status==host(words::outputChoiceFailed.get()));
        dialogResult=platform::Choice::Selected;
        dialogPath=pdfDir+"/report-"+tag+".pdf";
        exportPdf();assert(fs::is_regular_file(dialogPath) && initialFolder==afterDir);
        assert(suggestedFile.ends_with(".pdf") && status.find(dialogPath)!=std::string::npos);
        dialogPath=movieDir+"/vídeo-"+tag+".mp4";
        render();assert(workerCalls==workers+1 && exportThread && exportCode==ExportOk);
        assert(initialFolder==afterDir && suggestedFile.ends_with(".mp4") && fs::is_regular_file(dialogPath));
        exportThread=nullptr;assert(storedFolder==afterDir);
    }
    auto contents=[](const std::string& name){std::ifstream f(name,std::ios::binary);return std::string(std::istreambuf_iterator<char>(f),{});};
    dialogPath=finalDir+"/existente.pdf";std::ofstream(dialogPath)<<"ORIGINAL";
    overwrite=false;exportPdf();assert(contents(dialogPath)=="ORIGINAL" && confirmCalls>0);
    overwrite=true;failCommit=true;exportPdf();assert(contents(dialogPath)=="ORIGINAL");failCommit=false;
    exportPdf();assert(contents(dialogPath).starts_with("%PDF-"));
    dialogPath=finalDir+"/corrida.pdf";ExportTarget race;
    assert(selectExport(platform::OutputKind::Pdf,race)&&!race.replace);
    std::ofstream(dialogPath)<<"CRIADO DEPOIS";assert(!writePdf(saved,race)&&contents(dialogPath)=="CRIADO DEPOIS");
    dialogPath=finalDir+"/existente.mp4";std::ofstream(dialogPath)<<"VIDEO ORIGINAL";overwrite=false;
    const int oldWorkers=workerCalls;render();assert(workerCalls==oldWorkers && contents(dialogPath)=="VIDEO ORIGINAL");
    overwrite=true;ffmpegAvailable=false;render();assert(exportCode==ExportDecoder && contents(dialogPath)=="VIDEO ORIGINAL");
    exportThread=nullptr;ffmpegAvailable=true;
    std::ofstream(afterDir+"/clipe-2.qpc",std::ios::binary|std::ios::trunc).put('x');
    assert(exportComparison(movieTarget)==ExportMedia && contents(dialogPath)=="VIDEO ORIGINAL");
    dialogPath=finalDir+"/sobrescrever.cad";ExportTarget invalid;
    assert(!selectExport(platform::OutputKind::Pdf,invalid) && !fs::exists(dialogPath));
    const int previousPickers=pickerCalls;
    stats[1].n=0;clip[1].clear();
    render();exportPdf();
    assert(pickerCalls==previousPickers);
    for(auto& entry:fs::recursive_directory_iterator(dir)) assert(!entry.path().filename().string().starts_with(".circe-"));
    puts("Destino: gravação com dois fotogramas de teste, escolha por utilizador, reabertura de clipes em duas pastas, Guardar como PT/EN, cancelamento, falha e substituição verificados.");
    auto corrupt=saved;corrupt.summary[1].fps=NAN;assert(!validDocument(corrupt));
    session(dir,1,60);assert(host(pairPath.buf)!=host(original.buf)&&disk(original).summary[1].n==750);
    auto englishPair=pairPath;
    assert(host(englishPair.buf).find("comparison-")!=std::string::npos);
    language::active=language::Id::Portuguese;
    auto englishDocument=disk(englishPair);
    assert(validDocument(englishDocument) && englishDocument.summary[1].n==900);
    session(dir,0,120);assert(!saved.summary[1].n&&host(pairPath.buf)!=host(original.buf));
    assert(disk(original).summary[1].n==750);
    puts("Sessões: falha de escrita simulada preserva a comparação; novo Antes cria outro par. Exportação: dois vídeos com tempos diferentes alinhados e codificados por FFmpeg.");
}
