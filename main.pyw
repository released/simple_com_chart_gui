"""Compatible Python launcher; application modules live under src/python."""
from pathlib import Path
import runpy
import sys

if __name__ == '__main__':
    source = Path(__file__).resolve().parent / 'src' / 'python'
    sys.path.insert(0, str(source))
    runpy.run_path(str(source / 'main.pyw'), run_name='__main__')
