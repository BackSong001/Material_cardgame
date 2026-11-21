#include "../src/game.hpp"
#include <cassert>
#include <set>
int main() {
    using namespace material;
    std::array<int,6> counts{};
    std::set<std::pair<std::vector<double>,std::vector<double>>> spectra;
    for(const auto& c:cards) {
        ++counts.at(c.kind);
        assert(c.row>=2 && c.x.size()==c.y.size() && spectra.emplace(c.x,c.y).second);
    }
    assert(std::count(counts.begin(),counts.end(),4)==5);
    Card tied{0,L"Cu",2,{400,440,600},{1,9,9}};
    assert(tied.peak()==1);
    Game g;
    for(int i=19;i>=0;--i) {
        g.select(i); assert(g.slot==i && !g.back);
        g.flip(); assert(g.back);
        g.select(-1); assert(!g.back && g.slot==-1);
    }
}
