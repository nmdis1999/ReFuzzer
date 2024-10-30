import subprocess
import sys

if len(sys.argv) != 2:
    print("Usage: generatePrograms.py <number of runs>")
    sys.exit(1)

n = int(sys.argv[1])
for _ in range(n):
    subprocess.run(["./query_generator", "1"])
