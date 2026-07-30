#!/usr/bin/env python3
import pathlib
import subprocess
import sys


def unexpected_stderr(text: str) -> str:
    """ScriptBench may print zefc-bench phase timers to stderr; other stderr is a fail."""
    bad = []
    for line in text.splitlines(keepends=True):
        if line.startswith("zefc-bench:"):
            continue
        bad.append(line)
    return "".join(bad)


def main() -> int:
    exe = sys.argv[1]
    case = sys.argv[2]
    golden_path = pathlib.Path(sys.argv[3])
    golden = golden_path.read_text(encoding="utf-8")
    proc = subprocess.run([exe, case], capture_output=True, text=True, check=False)
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr)
        sys.stderr.write(f"exit code {proc.returncode}\n")
        return proc.returncode
    if proc.stdout != golden:
        sys.stderr.write("stdout mismatch for case %r\n" % case)
        sys.stderr.write("expected (%r):\n%s" % (golden, golden))
        sys.stderr.write("got (%r):\n%s" % (proc.stdout, proc.stdout))
        return 1
    bad = unexpected_stderr(proc.stderr or "")
    if bad:
        sys.stderr.write("expected empty stderr (aside from zefc-bench:), got:\n%s" % bad)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
