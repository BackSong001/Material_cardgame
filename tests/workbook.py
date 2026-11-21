import itertools
import math
import sys
import tempfile
import unittest
from pathlib import Path
from openpyxl import Workbook
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from prepare import DEFAULT_WORKBOOK, KINDS, display_shape, workbook


class DeckTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.path = Path(tmp.name)/'test.xlsx'
        self.book = Workbook()
        self.book.remove(self.book.active)
        for i,k in enumerate(KINDS):
            sheet = self.book.create_sheet(k)
            sheet.append(list(range(960,199,-40)))
            for j in [0,0,1,2,3]:
                sheet.append([int(column == 4*i+j) for column in range(20)])

    def read(self):
        self.book.save(self.path)
        return workbook(self.path)

    def test_mapping(self):
        cards = self.read()
        self.assertEqual(len(cards),20)
        row = next(c for c in cards if c[:3] == ('W','W',4))
        self.assertEqual(row[3],tuple(range(200,961,40)))
        self.assertEqual(row[4].index(1),14)

    def test_release_workbook(self):
        path = DEFAULT_WORKBOOK
        self.assertEqual(path.name,'testset_5_x100.xlsx')
        cards = workbook(path)
        self.assertEqual(cards, workbook(path))
        shapes = [display_shape(x,y) for _,_,_,x,y in cards]
        for a,b in itertools.combinations(shapes,2):
            self.assertGreaterEqual(math.dist(a,b)/math.sqrt(len(a)),0.08)
        from openpyxl import load_workbook
        original = load_workbook(path,read_only=True,data_only=True)
        try:
            rows = list(original['Sheet1'].values)
            for kind,sheet,line,x,y in cards:
                self.assertEqual(sheet,'Sheet1')
                self.assertEqual(rows[line-1][rows[0].index('Material')],kind)
                self.assertEqual(x,tuple(float(v[:-2]) for v in rows[0][:500]))
                self.assertEqual(y,tuple(rows[line-1][:500]))
            for kind in KINDS:
                self.assertEqual(sum(record[0]==kind for record in cards),4)
        finally:
            original.close()

    def test_collision(self):
        for column,cell in enumerate(self.book['Cu'][2],1):
            self.book['W'].cell(2,column).value = cell.value
        with self.assertRaisesRegex(ValueError,'W!2'): self.read()

    def test_short(self):
        self.book['Cu'].delete_rows(6)
        with self.assertRaisesRegex(ValueError,'Cu'): self.read()

    def test_scaled_copies(self):
        sheet = self.book['Cu']
        for row in range(2,7):
            for cell in sheet[row]:
                cell.value = row if cell.column == 1 else 0
        with self.assertRaisesRegex(ValueError,'최소 곡선 차이'):
            self.read()

    def test_missing(self):
        self.book['Cu']['C3'].value=None
        with self.assertRaisesRegex(ValueError,'Cu!3'): self.read()

    def test_header(self):
        self.book['Al']['C1']=960
        with self.assertRaisesRegex(ValueError,'Al!1'): self.read()


if __name__=='__main__':
    unittest.main()
