"""Tests for HENACY's read-only PS4 home menu database inspector."""
import importlib.util
from pathlib import Path
import sqlite3
import tempfile
import unittest

source = Path(__file__).resolve().parents[1] / 'scripts' / 'inspect-home-tiles.py'
spec = importlib.util.spec_from_file_location('home_tile_inspector', source)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class HomeTileInspectionTest(unittest.TestCase):
    def test_read_only_and_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'app.db'
            con = sqlite3.connect(path)
            con.execute('CREATE TABLE tbl_appbrowse_00000001 (titleId TEXT, titleName TEXT, visible INTEGER, sortPriority INTEGER)')
            con.executemany('INSERT INTO tbl_appbrowse_00000001 VALUES (?, ?, ?, ?)', [
                ('NPXS00000', 'PlayStation Store', 1, 1),
                ('HNCY00001', 'HENACY v1', 1, 2),
                ('CUSA00000', 'Another game', 1, 3),
            ])
            con.commit()
            con.close()
            before = path.read_bytes()
            report = module.scan(path)
            self.assertTrue(report['read_only'])
            self.assertEqual(report['quick_check'], 'ok')
            self.assertEqual(len(report['matches']), 2)
            self.assertEqual(path.read_bytes(), before)
            self.assertIn('tbl_appbrowse_00000001', [item['table'] for item in report['candidate_home_tables']])

    def test_does_not_create_missing_database(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'does_not_exist.db'
            with self.assertRaises(ValueError):
                module.scan(path)
            self.assertFalse(path.exists())


if __name__ == '__main__':
    unittest.main()
