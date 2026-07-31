#!/usr/bin/env python3
"""Run a compiled case that must exit nonzero; compare stderr (and optional stdout) to goldens."""
import pathlib
import subprocess
import sys


def main() -> int:
    exe = sys.argv[1]
    stderr_golden_path = pathlib.Path(sys.argv[2])
    stderr_golden = stderr_golden_path.read_text(encoding="utf-8")
    stdout_golden = None
    if len(sys.argv) > 3:
        stdout_golden = pathlib.Path(sys.argv[3]).read_text(encoding="utf-8")
    proc = subprocess.run([exe], capture_output=True, text=True, check=False)
    if proc.returncode == 0:
        sys.stderr.write("expected nonzero exit\n")
        sys.stderr.write("stdout: %r\nstderr: %r\n" % (proc.stdout, proc.stderr))
        return 1
    if stdout_golden is None:
        if proc.stdout:
            sys.stderr.write("expected empty stdout, got:\n%s" % proc.stdout)
            return 1
    elif proc.stdout != stdout_golden:
        sys.stderr.write("stdout mismatch\n")
        sys.stderr.write("expected (%r):\n%s" % (stdout_golden, stdout_golden))
        sys.stderr.write("got (%r):\n%s" % (proc.stdout, proc.stdout))
        return 1
    if proc.stderr != stderr_golden:
        sys.stderr.write("stderr mismatch\n")
        sys.stderr.write("expected (%r):\n%s" % (stderr_golden, stderr_golden))
        sys.stderr.write("got (%r):\n%s" % (proc.stderr, proc.stderr))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
