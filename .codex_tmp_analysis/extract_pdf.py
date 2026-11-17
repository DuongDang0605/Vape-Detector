from pathlib import Path
import sys
from pypdf import PdfReader

source = Path(r"C:\Fschool_IoT\main.pdf")
reader = PdfReader(source)
sys.stdout.reconfigure(encoding="utf-8")
start = int(sys.argv[1]) if len(sys.argv) > 1 else 1
end = int(sys.argv[2]) if len(sys.argv) > 2 else len(reader.pages)
for index, page in enumerate(reader.pages, start=1):
    if index < start or index > end:
        continue
    print(f"\n===== PAGE {index} / {len(reader.pages)} =====\n")
    print(page.extract_text() or "[NO EXTRACTABLE TEXT]")
