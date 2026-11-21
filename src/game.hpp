#pragma once
#include <algorithm>
#include <array>
#include <numeric>
#include <random>
#include <vector>

namespace material {
struct Card {
    int kind;
    const wchar_t* sheet;
    int row;
    std::vector<double> x, y;
    size_t peak() const { return std::max_element(y.begin(),y.end())-y.begin(); }
};
#include "deck.inc"
constexpr const wchar_t* names[]{L"카파 (Cu)",L"텅스텐 (W)",L"타이타늄 (Ti)",L"실리콘 옥사이드 (SiO₂)",L"실리콘 나이트라이드 (Si₃N₄)"};
struct Game {
    std::array<int,20> order{};
    std::array<bool,20> seen{};
    int slot=-1;
    bool back=false;
    void start(std::mt19937& rng) {
        std::iota(order.begin(),order.end(),0);
        std::shuffle(order.begin(),order.end(),rng);
        seen.fill(false);
        select(-1);
    }
    void select(int n) { slot=n; back=false; }
    void flip() { back=!back; if(back) seen[slot]=true; }
    int count() const { return std::count(seen.begin(),seen.end(),true); }
};
}
