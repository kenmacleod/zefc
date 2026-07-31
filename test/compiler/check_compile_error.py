#!/usr/bin/env python3
"""Run zefc on a .zef that must fail; compare stderr to a golden."""
import pathlib
import subprocess
import sys
import tempfile


def main() -> int:
    zefc = sys.argv[1]
    src = sys.argv[2]
    golden_path = pathlib.Path(sys.argv[3])
    golden = golden_path.read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory() as tmp:
        out = pathlib.Path(tmp) / "out.cpp"
        proc = subprocess.run(
            [zefc, src, "-o", str(out)],
            capture_output=True,
            text=True,
            check=False,
        )
    if proc.returncode == 0:
        sys.stderr.write("expected nonzero exit from zefc for %r\n" % src)
        sys.stderr.write("stdout: %r\nstderr: %r\n" % (proc.stdout, proc.stderr))
        return 1
    if proc.stdout:
        sys.stderr.write("expected empty stdout, got:\n%s" % proc.stdout)
        return 1
    if proc.stderr != golden:
        sys.stderr.write("stderr mismatch for %r\n" % src)
        sys.stderr.write("expected (%r):\n%s" % (golden, golden))
        sys.stderr.write("got (%r):\n%s" % (proc.stderr, proc.stderr))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
