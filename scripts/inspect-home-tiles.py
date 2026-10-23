#!/usr/bin/env python3
"""Read-only PS4 home tile inventory from an exported app.db SQLite database.

This script never edits app.db, a payload, or firmware. It is useful for planning a
reversible HENACY launcher replacement once a real PS4 database is available.
Usage: python3 inspect_home_tiles.py /path/to/app.db [--json]
"""
import argparse
import json
import os
from pathlib import Path
import sqlite3
import sys

TERMS = ("playstation store", "ps store", "henacy", "hncy")
RELEVANT_COLS = ("title", "name", "content", "description", "uri", "path", "app", "id")


def quoted(name: str) -> str:
    return '"' + name.replace('"', '""') + '"'


def scan(db: Path) -> dict:
    if not db.is_file():
        raise ValueError('Database does not exist: ' + str(db))
    # SQLite mode=ro: explicitly disallow creating/updating any database files.
    uri = db.resolve().as_uri() + '?mode=ro'
    con = sqlite3.connect(uri, uri=True, timeout=5)
    try:
        con.row_factory = sqlite3.Row
        con.execute('PRAGMA query_only = ON')
        integrity = con.execute('PRAGMA quick_check').fetchone()[0]
        table_rows = con.execute("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name").fetchall()
        result = {'file': str(db), 'read_only': True, 'quick_check': integrity, 'tables_checked': 0, 'matches': [], 'candidate_home_tables': []}
        for (table,) in table_rows:
            columns = [r['name'] for r in con.execute('PRAGMA table_info(' + quoted(table) + ')')]
            if not columns:
                continue
            result['tables_checked'] += 1
            lowered = table.lower()
            if any(term in lowered for term in ('appbrowse', 'appinfo', 'home', 'menu', 'browse')):
                result['candidate_home_tables'].append({'table': table, 'columns': columns})
            candidates = [c for c in columns if any(key in c.lower() for key in RELEVANT_COLS)]
            if not candidates:
                continue
            # App metadata values can have arbitrary keys; only scan text-ish
            # columns and cap results to avoid dumping personal database data.
            pred = ' OR '.join('lower(CAST(' + quoted(c) + ' AS TEXT)) LIKE ?' for c in candidates for _ in TERMS)
            args = tuple('%' + term + '%' for _ in candidates for term in TERMS)
            query = 'SELECT * FROM ' + quoted(table) + ' WHERE ' + pred + ' LIMIT 20'
            for record in con.execute(query, args):
                row = dict(record)
                relevant = {}
                for col, val in row.items():
                    if isinstance(val, (bytes, bytearray)):
                        continue
                    if col in candidates or any(x in col.lower() for x in ('visible', 'hidden', 'sort', 'priority')):
                        relevant[col] = str(val)[:180] if val is not None else None
                result['matches'].append({'table': table, 'fields': relevant})
        return result
    finally:
        con.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('database', type=Path, help='An offline copy of PS4 /system_data/priv/mms/app.db')
    parser.add_argument('--json', action='store_true', help='Output machine-readable JSON')
    args = parser.parse_args()
    try:
        result = scan(args.database)
    except (ValueError, sqlite3.Error, OSError) as exc:
        parser.exit(2, 'Unable to inspect database: ' + str(exc) + '\n')
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print('HENACY home-screen tile audit (READ ONLY)')
        print('SQLite quick_check:', result['quick_check'])
        print('Tables checked:', result['tables_checked'])
        print('Potential home tables:', ', '.join(x['table'] for x in result['candidate_home_tables']) or 'none')
        print('Matching metadata records:', len(result['matches']))
        for entry in result['matches']:
            print('\n' + entry['table'] + ':')
            for key, value in entry['fields'].items():
                print('  ' + key + ': ' + str(value))
        print('\nNo changes made. A Store-tile redirect requires a verified HENACY launcher and an offline-tested restore procedure.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
