import subprocess
import sys
import threading
import time

duration = 24 * 60 * 60  # 24 hours
timer = threading.Timer(duration, sys.exit)
timer.start()

try:
    if len(sys.argv) != 2:
        print("Usage: generatePrograms.py <number of runs>")
        sys.exit(1)

    n = int(sys.argv[1])
    start_time = time.time()
    while True:
        elapsed_time = time.time() - start_time
        if elapsed_time >= duration:
            break

        subprocess.run(["./query_generator", "1"])

        time.sleep(1)
finally:
    timer.cancel()
