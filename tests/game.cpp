#include "../src/game.hpp"
#include <cassert>
#include <set>
int main() {
    using namespace material;
    std::array<int,5> counts{};
    std::set<std::pair<std::vector<double>,std::vector<double>>> spectra;
    for(const auto& c:cards) {
        ++counts.at(c.kind);
        assert(c.row>=2 && c.x.size()==c.y.size() && spectra.emplace(c.x,c.y).second);
    }
    for(int count:counts) assert(count==4);
    Card tied{0,L"Cu",2,{400,440,600},{1,9,9}};
    assert(tied.peak()==1);
    Game g; std::mt19937 rng(42); g.start(rng);
    assert(std::set<int>(g.order.begin(),g.order.end()).size()==20);
    for(int i=19;i>=0;--i) {
        g.select(i); assert(!g.back && !g.seen[i]);
        g.select(-1); assert(g.count()==19-i);
        g.select(i); g.flip(); g.flip(); g.flip(); assert(g.count()==20-i);
    }
    g.start(rng); assert(g.slot==-1 && g.count()==0);
}
