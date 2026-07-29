#!/usr/bin/env python3
import pathlib
import subprocess
import sys


def main() -> int:
    exe = sys.argv[1]
    case = sys.argv[2]
    golden_path = pathlib.Path(sys.argv[3])
    golden = golden_path.read_text(encoding="utf-8")
    proc = subprocess.run([exe, case], capture_output=True, text=True, check=False)
    if proc.returncode == 0:
        sys.stderr.write("expected nonzero exit for case %r\n" % case)
        sys.stderr.write("stdout: %r\nstderr: %r\n" % (proc.stdout, proc.stderr))
        return 1
    if proc.stdout:
        sys.stderr.write("expected empty stdout, got:\n%s" % proc.stdout)
        return 1
    if proc.stderr != golden:
        sys.stderr.write("stderr mismatch for case %r\n" % case)
        sys.stderr.write("expected (%r):\n%s" % (golden, golden))
        sys.stderr.write("got (%r):\n%s" % (proc.stderr, proc.stderr))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
