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
constexpr const wchar_t* names[]{L"Copper", L"Tungsten", L"Aluminum", L"Silicon Dioxide", L"Silicon Nitride", L"Titanium"};
struct Game {
    int slot=-1;
    bool back=false;
    void select(int n) { slot=n; back=false; }
    void flip() { back=!back; }
};
}
