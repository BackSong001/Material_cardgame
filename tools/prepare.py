"""Build the embedded spectrum deck and Win32 palette."""
from bisect import bisect_left
import re
import json
import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
KINDS = ['Cu', 'W', 'Al', 'SiO2', 'Si3N4']
LEGACY = ['Cu', 'W', 'Ti', 'SiO2', 'Si3N4']


def numbers(cells, where):
    try:
        values = tuple(float(v) for v in cells if not isinstance(v, bool))
        if len(values) != len(cells) or not all(map(math.isfinite, values)):
            raise ValueError()
        return values
    except (TypeError, ValueError):
        raise ValueError(f'{where}: 숫자가 누락되었거나 유효하지 않습니다.') from None


# Compare vertical positions at roughly one sample per thumbnail pixel.
DISPLAY_GRID = range(200, 1001, 4)
MIN_RMS_GAP = 0.08
DEFAULT_WORKBOOK = ROOT/'testset_5_x100.xlsx'


def display_shape(x, y):
    low, high = min(0., min(y)), max(0., max(y))
    height = high - low or 1.
    shape = []
    for wave in DISPLAY_GRID:
        i = bisect_left(x, wave)
        if i == len(x) or wave < x[0]:
            shape.append(0.)
            continue
        value = y[i]
        if wave != x[i]:
            fraction = (wave - x[i-1]) / (x[i] - x[i-1])
            value = y[i-1] + fraction * (y[i] - y[i-1])
        shape.append((value - low) / height)
    return shape


def select_deck(candidates, kinds):
    shapes = [display_shape(x, y) for _, _, _, x, y in candidates]
    groups = {k: [] for k in kinds}
    for index, card in enumerate(candidates):
        if card[0] not in groups:
            raise ValueError(f'{card[0]}: 덱 구성에 없는 물질입니다.')
        groups[card[0]].append(index)
    for kind, group in groups.items():
        if len(group) < 4:
            raise ValueError(f'{kind}: 고유 샘플 4개가 필요합니다.')

    # Try one starting point per material so file order has less influence.
    best_gap, best = -1., []
    for group in groups.values():
        chosen = []
        counts = dict.fromkeys(kinds, 0)
        nearest = [math.inf] * len(candidates)
        index = group[0]
        gap = math.inf
        while True:
            chosen.append(index)
            counts[candidates[index][0]] += 1
            if len(chosen) == 4 * len(kinds):
                break
            eligible = [i for k, members in groups.items() if counts[k] < 4
                        for i in members if i not in chosen]
            for i in eligible:
                nearest[i] = min(nearest[i], math.dist(shapes[i], shapes[index]))
            index = max(eligible, key=nearest.__getitem__)
            gap = min(gap, nearest[index])
        if gap > best_gap:
            best_gap, best = gap, chosen
    rms = best_gap / math.sqrt(len(DISPLAY_GRID))
    if rms < MIN_RMS_GAP:
        raise ValueError(f'선정된 카드의 최소 곡선 차이 {rms:.1%}가 기준 {MIN_RMS_GAP:.0%} 미만입니다.')
    return [candidates[i] for i in best]


def workbook(path):
    from openpyxl import load_workbook
    book = load_workbook(path, read_only=True, data_only=True)
    candidates, seen = [], {}
    def extract(sheet, header, rows, fixed_kind=None):
        if fixed_kind is None:
            material = header.index('Material')
            columns = [(i, float(match[1])) for i, name in enumerate(header)
                       if (match := re.fullmatch(r'([0-9]+(?:\.[0-9]+)?)\s*nm', str(name).strip()))]
        else:
            while header and header[-1] is None:
                header.pop()
            columns = list(enumerate(numbers(header, f'{sheet}!1')))
        columns.sort(key=lambda pair: pair[1])
        x = tuple(w for _, w in columns)
        if len(set(x)) != len(x) or len(x) < 2:
            raise ValueError(f'{sheet}!1: 서로 다른 파장 두 개 이상이 필요합니다.')
        for line, row in enumerate(rows, 2):
            if all(v is None for v in row):
                continue
            kind = fixed_kind or row[material]
            where = f'{sheet}!{line}'
            if kind not in KINDS + LEGACY:
                raise ValueError(f'{where}: 알 수 없는 물질 {kind}')
            if fixed_kind and any(v is not None for v in row[len(x):]):
                raise ValueError(f'{where}: 헤더 없는 열입니다.')
            y = numbers([row[i] for i,_ in columns], where)
            key = x, y
            if key in seen:
                other, origin = seen[key]
                if other != kind:
                    raise ValueError(f'{where}: {origin}와 동일한 스펙트럼입니다.')
                continue
            seen[key] = kind, where
            candidates.append((kind,sheet,line,x,y))
    try:
        if len(book.sheetnames) == 1:
            sheet = book.active
            rows = iter(sheet.values)
            header = list(next(rows, ()))
            if header.count('Material') != 1:
                raise ValueError(f'{sheet.title}!1: Material 열 하나가 필요합니다.')
            extract(sheet.title, header, rows)
            kinds = KINDS
        else:
            kinds = LEGACY if 'Ti' in book.sheetnames else KINDS
            for kind in kinds:
                sheets = [s for s in book.sheetnames if s.translate(str.maketrans('₂₃₄','234')) == kind]
                if len(sheets) != 1:
                    raise ValueError(f'{kind}: 해당 시트가 정확히 하나 필요합니다.')
                rows = iter(book[sheets[0]].values)
                extract(sheets[0], list(next(rows, ())), rows, kind)
    finally:
        book.close()
    # Fixed exhibition layout. Separate adjacent samples of the same material.
    positions = [0,5,10,15,16,9,18,3,12,7,14,1,17,6,11,19,8,13,4,2]
    by_kind = sorted(select_deck(candidates, kinds), key=lambda card: kinds.index(card[0]))
    return [by_kind[i] for i in positions]


def generate(dest, source=DEFAULT_WORKBOOK):
    cards = workbook(source)
    def vector(values):
        return '{' + ','.join(map(repr,values)) + '}'
    kinds = KINDS + ["Ti"]
    entries = [f'{{{kinds.index(k)},L{json.dumps(sheet)},{line},{vector(x)},{vector(y)}}}' for k,sheet,line,x,y in cards]
    (dest/'deck.inc').write_text('const std::array<Card,20> cards{{\n'+',\n'.join(entries)+'\n}};\n')
    palette = json.loads((ROOT/'colors.json').read_text())
    (dest/'palette.inc').write_text('\n'.join(f'constexpr COLORREF {k}=RGB({",".join(map(str,bytes.fromhex(v[1:]))) });' for k,v in palette.items()))


if __name__ == '__main__':
    generate(Path(sys.argv[1]), *sys.argv[2:])
