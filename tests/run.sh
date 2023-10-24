#!/usr/bin/env bash
# Usage: tests/run.sh <path-to-FileSystem-binary> [--record]
# Runs every scenario in tests/scenarios against the binary in a clean temp dir and
# compares stdout (CR stripped) plus files left in the working dir with tests/golden.
set -u

BIN=${1:?usage: run.sh <binary> [--record]}
MODE=${2:-compare}
HERE=$(cd "$(dirname "$0")" && pwd)
BIN=$(cd "$(dirname "$BIN")" && pwd)/$(basename "$BIN")
fail=0

for scenario in "$HERE"/scenarios/*.in; do
    name=$(basename "$scenario" .in)
    work=$(mktemp -d)
    cp "$BIN" "$work/fs.bin" && chmod +x "$work/fs.bin"
    cp "$HERE"/fixtures/* "$work"/ 2>/dev/null
    [ -f "${scenario%.in}.setup" ] && (cd "$work" && sh "${scenario%.in}.setup")

    actual="$work/actual.out"
    : > "$actual"
    # A line "#restart" splits the scenario into separate runs over the same disk file.
    awk -v dir="$work" 'BEGIN{n=0} /^#restart$/{n++; next} {print > (dir "/part" n ".in")}' "$scenario"
    for part in $(ls "$work"/part*.in | sort -V); do
        (cd "$work" && timeout 20 ./fs.bin < "$part" 2>&1 | tr -d "\r" | head -c 2000 >> "$actual"
         code=${PIPESTATUS[0]}; printf "\n[exit=%s]\n" "$code" >> "$actual")
    done
    (cd "$work" && for f in $(ls -A | grep -vE '^(fs\.bin|disk\.bin|actual\.out|part[0-9]+\.in)$' | sort); do
         echo "--- file: $f" >> "$actual"
         if [ "$(wc -c < "$f")" -le 256 ]; then xxd "$f" >> "$actual"; else echo "$(wc -c < "$f") bytes, cksum $(cksum < "$f")" >> "$actual"; fi
     done)

    golden="$HERE/golden/$name.out"
    if [ "$MODE" = "--record" ]; then
        cp "$actual" "$golden"
        echo "RECORDED $name"
    elif diff -u "$golden" "$actual" > "$work/diff.txt"; then
        echo "PASS     $name"
    else
        echo "FAIL     $name"
        cat "$work/diff.txt"
        fail=1
    fi
    rm -rf "$work"
done

exit $fail
