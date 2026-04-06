
from pathlib import Path
import re

src = Path("dict_code_usable.txt")
out_dict = Path("dict_sorted.txt")
out_idx = Path("dict_index.txt")

def normalize_word(word: str) -> str:
    return word.strip().lower()

def bucket_key(word: str) -> str:
    word = normalize_word(word)
    if not word:
        return "__"
    a = word[0] if 'a' <= word[0] <= 'z' else '_'
    if len(word) > 1 and 'a' <= word[1] <= 'z':
        b = word[1]
    else:
        b = '_'
    return a + b

lines = []
with src.open("r", encoding="utf-8", errors="ignore") as f:
    for raw in f:
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        word, meaning = line.split("=", 1)
        word = word.strip()
        meaning = re.sub(r"\s+", " ", meaning.strip())
        if word and meaning:
            lines.append((normalize_word(word), word, meaning))

lines.sort(key=lambda x: x[0])

offsets = {}
current_offset = 0
with out_dict.open("w", encoding="utf-8", newline="\n") as f:
    for _, word, meaning in lines:
        row = f"{word}={meaning}\n"
        key2 = bucket_key(word)
        key1 = bucket_key(word[:1])
        if key2 not in offsets:
            offsets[key2] = [current_offset, current_offset]
        offsets[key2][1] = current_offset + len(row.encode("utf-8"))
        if key1 not in offsets:
            offsets[key1] = [current_offset, current_offset]
        offsets[key1][1] = current_offset + len(row.encode("utf-8"))
        f.write(row)
        current_offset += len(row.encode("utf-8"))

with out_idx.open("w", encoding="utf-8", newline="\n") as f:
    for key in sorted(offsets):
        start, end = offsets[key]
        f.write(f"{key}={start}:{end}\n")

print(f"Wrote {out_dict} and {out_idx} with {len(lines)} entries.")
