#!/usr/bin/env python3
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
        return proc.returncode
    if proc.stdout != golden:
        sys.stderr.write("stdout mismatch\n")
        sys.stderr.write("expected (%r):\n%s" % (golden, golden))
        sys.stderr.write("got (%r):\n%s" % (proc.stdout, proc.stdout))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
