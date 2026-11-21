#pragma once
#include <algorithm>
#include <array>

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
constexpr const wchar_t* names[]{L"Cu", L"W", L"Al", L"SiO₂", L"Si₃N₄", L"Ti"};
struct Game {
    int slot=-1;
    bool back=false;
    void select(int n) { slot=n; back=false; }
    void flip() { back=!back; }
};
}
