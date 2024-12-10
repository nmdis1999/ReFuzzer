import os
import shutil
import subprocess
import sys
import threading
import time
from datetime import datetime

def create_folder(stage, model1, model2):
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    folder_name = f"{stage}_{model1}_{model2}_{timestamp}"
    os.makedirs(folder_name, exist_ok=True)
    return folder_name

def move_files(folder_name):
    for path in ["../log", "../test", "../prompt"]:
        if os.path.exists(path):
            dest = os.path.join(folder_name, os.path.basename(path))
            shutil.copytree(path, dest)

def make_zip(folder_name):
    shutil.make_archive(folder_name, "zip", folder_name)

def main():
    duration = 24 * 60 * 60
    timer = threading.Timer(duration, sys.exit)
    timer.start()

    try:
        if len(sys.argv) != 5:
            print("Usage: generatePrograms.py <number_of_runs> <stage> <model1> <model2>")
            sys.exit(1)

        n = int(sys.argv[1])
        stage, model1, model2 = sys.argv[2], sys.argv[3], sys.argv[4]
        folder_name = create_folder(stage, model1, model2)
        start_time = time.time()
        run_count = 0

        while True:
            elapsed_time = time.time() - start_time
            if elapsed_time >= duration or run_count >= n:
                break

            subprocess.run(["./query_generator", "1"])

            original_dir = os.getcwd()
            os.chdir("../model2")
            subprocess.run(["./recompile"])

            os.chdir(original_dir)

            subprocess.run(["./query_generator", "2"])

            run_count += 1
            time.sleep(1)

        move_files(folder_name)
        make_zip(folder_name)
    finally:
        timer.cancel()

if __name__ == "__main__":
    main()
