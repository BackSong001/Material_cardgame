import sys
import tempfile
import unittest
from pathlib import Path
from openpyxl import Workbook
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from prepare import workbook, KINDS


class DeckTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.path = Path(tmp.name)/'test.xlsx'
        self.book = Workbook()
        self.book.remove(self.book.active)
        for i,k in enumerate(KINDS):
            sheet = self.book.create_sheet(k)
            sheet.append([650,405,511])
            for j in [0,0,1,2,3]:
                sheet.append([10*i+j,7,11])

    def read(self):
        self.book.save(self.path)
        return workbook(self.path)

    def test_mapping(self):
        cards = self.read()
        self.assertEqual(len(cards),20)
        self.assertEqual(cards[5],('W','W',4,(405.,511.,650.),(7.,11.,11.)))

    def test_collision(self):
        self.book['W']['A2']=0
        with self.assertRaisesRegex(ValueError,'W!2'): self.read()

    def test_short(self):
        self.book['Cu'].delete_rows(6)
        with self.assertRaisesRegex(ValueError,'Cu'): self.read()

    def test_missing(self):
        self.book['Cu']['C3'].value=None
        with self.assertRaisesRegex(ValueError,'Cu!3'): self.read()

    def test_header(self):
        self.book['Ti']['C1']=405
        with self.assertRaisesRegex(ValueError,'Ti!1'): self.read()


if __name__=='__main__':
    unittest.main()
