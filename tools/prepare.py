"""Build the embedded spectrum deck and Win32 palette."""
import csv
import json
import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
KINDS = ['Cu', 'W', 'Ti', 'SiO2', 'Si3N4']


def numbers(cells, where):
    try:
        values = tuple(float(v) for v in cells if not isinstance(v, bool))
        if len(values) != len(cells) or not all(map(math.isfinite, values)):
            raise ValueError()
        return values
    except (TypeError, ValueError):
        raise ValueError(f'{where}: 숫자가 누락되었거나 유효하지 않습니다.') from None


def workbook(path):
    from openpyxl import load_workbook
    book = load_workbook(path, read_only=True, data_only=True)
    selected, seen = [], {}
    try:
        for kind in KINDS:
            sheets = [s for s in book.sheetnames if s.translate(str.maketrans('₂₃₄','234')) == kind]
            if len(sheets) != 1:
                raise ValueError(f'{kind}: 해당 시트가 정확히 하나 필요합니다.')
            sheet = sheets[0]
            rows = iter(book[sheet].values)
            header = list(next(rows, ()))
            while header and header[-1] is None:
                header.pop()
            x = numbers(header, f'{sheet}!1')
            if len(set(x)) != len(x) or len(x) < 2:
                raise ValueError(f'{sheet}!1: 서로 다른 파장 두 개 이상이 필요합니다.')
            indices = sorted(range(len(x)), key=x.__getitem__)
            wavelengths = tuple(x[i] for i in indices)
            count = 0
            for line, row in enumerate(rows, 2):
                if all(v is None for v in row):
                    continue
                where = f'{sheet}!{line}'
                if any(v is not None for v in row[len(x):]):
                    raise ValueError(f'{where}: 헤더 없는 열입니다.')
                values = numbers(row[:len(x)], where)
                y = tuple(values[i] for i in indices)
                key = wavelengths, y
                if key in seen:
                    other, origin = seen[key]
                    if other != kind:
                        raise ValueError(f'{where}: {origin}와 동일한 스펙트럼입니다.')
                    continue
                seen[key] = kind, where
                if count < 4:
                    selected.append((kind,sheet,line,wavelengths,y))
                    count += 1
            if count < 4:
                raise ValueError(f'{sheet}: 고유 샘플이 {count}개입니다.')
    finally:
        book.close()
    return selected


def generate(dest, source=None):
    if source:
        cards = workbook(source)
    else:
        with (ROOT/'data/demo.csv').open() as f:
            rows = csv.reader(f)
            x = list(map(float,next(rows)[1:]))
            cards = [(r[0],'demo.csv',i,x,list(map(float,r[1:]))) for i,r in enumerate(rows,2)]
    def vector(values):
        return '{' + ','.join(map(repr,values)) + '}'
    entries = [f'{{{KINDS.index(k)},L{json.dumps(sheet)},{line},{vector(x)},{vector(y)}}}' for k,sheet,line,x,y in cards]
    (dest/'deck.inc').write_text('const bool demo = '+str(not source).lower()+';\nconst std::array<Card,20> cards{{\n'+',\n'.join(entries)+'\n}};\n')
    palette = json.loads((ROOT/'colors.json').read_text())
    (dest/'palette.inc').write_text('\n'.join(f'constexpr COLORREF {k}=RGB({",".join(map(str,bytes.fromhex(v[1:]))) });' for k,v in palette.items()))


if __name__ == '__main__':
    generate(Path(sys.argv[1]), *sys.argv[2:])
