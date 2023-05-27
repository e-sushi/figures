import os
import subprocess

repo_root = "/home/sushi/src/suugu"
repo_output = "/home/sushi/src/suugu/build/debug/pch"

os.chdir(repo_output)

headers = []

for root,dirs,files in os.walk(repo_root):
    path = root.split(os.sep)
    for file in files:
        if file.endswith(".h"):
            headers.append(f"{root}/{file}")


if not os.path.exists(repo_output):
    os.makedirs(repo_output)

f = open(f"{repo_output}/query.h", "w")

for h in headers:
    f.write(f"#include \"{h}\"\n")

subprocess.call(["clang++", "-cc1", "-std=c++17", "-x", "c++-header", f"{repo_output}/query.h", "-emit-pch", "-o", "query.pch"])

