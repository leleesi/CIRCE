#include "../src/layout.hpp"
#include "../src/text.hpp"
#include <cassert>
#include <cstdio>

static bool same(const wchar_t* left, const wchar_t* right) {
    while (*left && *left == *right) { ++left; ++right; }
    return *left == *right;
}
int main() {
    using language::Id;
    constexpr unsigned en = unsigned(Id::English), pt = unsigned(Id::Portuguese);
    for (unsigned os : {en, pt, 1046U, 1036U}) {
        assert(language::select(en, pt, os) == Id::English);
        assert(language::select(pt, en, os) == Id::Portuguese);
        assert(language::select(0, pt, os) == Id::Portuguese);
        assert(language::select(99, en, os) == Id::English);
    }
    assert(language::select(0,0,1046) == Id::Portuguese);
    assert(language::select(0,0,1036) == Id::English);
    assert(language::tag("pt\r\n",4)==pt && language::tag("en",2)==en && language::tag("en\n",3)==en);
    assert(!language::tag("",0) && !language::tag("p",1) && !language::tag("es",2) && !language::tag("en x",4));

    for (Id id : {Id::Portuguese, Id::English}) {
        language::active=id;
        const bool english=id==Id::English;
        assert(same(words::sessionTab.get(),english ? L"Session" : L"Sessão"));
        assert(same(words::recordAfter.get(),english ? L"Record After" : L"Registar Depois"));
        for (const auto& item : panelui::items) {
            assert(item.caption.get() == (english ? item.caption.en : item.caption.pt));
            assert((*item.caption.pt != 0) == (*item.caption.en != 0));
        }
        const auto filter=words::comparisonFilter.get();
        assert(same(filter,english ? L"CIRCE comparisons" : L"Comparações CIRCE"));
        const wchar_t* pattern=filter;
        while (*pattern++) {}
        assert(same(pattern,L"*.cad") && pattern[6]==0);
        Text<32> decimal;
        decimal.decimal(59.94);
        assert(same(decimal.buf,english ? L"59.9" : L"59,9"));
    }
    puts("Idiomas: prioridade da instalação, região do Windows, filtro de comparações, painel e casas decimais verificados.");
}
