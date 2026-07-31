#!/usr/bin/env python3
"""Compare stdout of a no-arg executable to a golden file."""
import pathlib
import subprocess
import sys


def main() -> int:
    exe = sys.argv[1]
    golden_path = pathlib.Path(sys.argv[2])
    golden = golden_path.read_text(encoding="utf-8")
    proc = subprocess.run([exe], capture_output=True, text=True, check=False)
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr)
        sys.stderr.write(f"exit code {proc.returncode}\n")
        return proc.returncode or 1
    if proc.stdout != golden:
        sys.stderr.write("stdout mismatch\n")
        sys.stderr.write("expected:\n%s" % golden)
        sys.stderr.write("got:\n%s" % proc.stdout)
        return 1
    if proc.stderr:
        sys.stderr.write("unexpected stderr:\n%s" % proc.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
