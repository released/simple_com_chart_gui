"""Read test-owned exports with an independent, optional BIFF8 reader.

xlrd is a verification-only dependency; do not bundle it with the application.
"""
import argparse
import csv
import pathlib
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reader-path", type=pathlib.Path, help="Isolated folder containing xlrd")
    parser.add_argument("--fixtures", type=pathlib.Path, default=pathlib.Path("build/verify/export-tests"))
    args = parser.parse_args()
    if args.reader_path:
        sys.path.insert(0, str(args.reader_path.resolve(strict=True)))
    import xlrd

    candidates = list(args.fixtures.glob("*/wide.xls"))
    if not candidates:
        raise RuntimeError("Build/run telemetry_core first to generate export fixtures")
    wide = max(candidates, key=lambda path: path.stat().st_mtime)
    sheet = xlrd.open_workbook(wide).sheet_by_index(0)
    assert (sheet.nrows, sheet.ncols) == (2, 256)
    for column in range(2, 256):
        name = sheet.cell_value(0, column)
        assert len(name) > 120
        assert sheet.cell_value(1, column) == int(name[1:name.index("x")]) + .125

    path = wide.with_name("capture.xls")
    sheet = xlrd.open_workbook(path).sheet_by_index(0)
    headers = sheet.row_values(0)
    assert (sheet.nrows, sheet.ncols) == (5, 8)
    assert sheet.cell_value(1, headers.index("small")) == -.0025
    assert sheet.cell_value(2, headers.index("small")) == ""
    assert sheet.cell_value(4, headers.index("\u6eab\u5ea6")) == 26.5
    assert sheet.cell_type(4, headers.index("=FORMULA")) == xlrd.XL_CELL_NUMBER
    with path.with_suffix(".csv").open(encoding="utf-8-sig", newline="") as file:
        rows = list(csv.reader(file))
    assert len(rows) == 5 and len(rows[0]) == 8
    assert "'=FORMULA" in rows[0] and 'quote"field' in rows[0]
    assert float(rows[1][rows[0].index("small")]) == -.0025
    assert rows[2][rows[0].index("small")] == ""
    assert path.with_suffix(".csv").read_bytes() == path.with_suffix(".cvs").read_bytes()
    assert path.with_suffix(".log").read_bytes() == path.with_suffix(".txt").read_bytes()
    print("PASS: independent BIFF8/CSV readback, 256 columns/SST continuations, Unicode, numbers, blanks, aliases")
    print("Evidence:", wide.parent)


if __name__ == "__main__":
    main()
