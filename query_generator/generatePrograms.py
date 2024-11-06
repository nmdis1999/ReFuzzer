import subprocess
import sys
import threading

timer = threading.Timer(60, sys.exit)
timer.start()
try:
    if len(sys.argv) != 2:
        print("Usage: generatePrograms.py <number of runs>")
        sys.exit(1)

    n = int(sys.argv[1])
    for _ in range(n):
        subprocess.run(["./query_generator", "1"])
finally:
    timer.cancel()
