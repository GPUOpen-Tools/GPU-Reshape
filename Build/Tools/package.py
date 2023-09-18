# 
# The MIT License (MIT)
# 
# Copyright (c) 2023 Miguel Petersen
# Copyright (c) 2023 Advanced Micro Devices, Inc
# Copyright (c) 2023 Avalanche Studios Group
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy 
# of this software and associated documentation files (the "Software"), to deal 
# in the Software without restriction, including without limitation the rights 
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
# of the Software, and to permit persons to whom the Software is furnished to do so, 
# subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all 
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 

import os
import sys
import shutil

# Paths
bin_dir_root = os.path.join("../", "../", "Bin")
pck_dir_root = os.path.join("../", "../", "Package")

# Ignore list
ignore_list = [
    # Packaged separately
    ".pdb",
    
    # Executables, handled through required files
    # Mostly unit testing, build time tooling
    ".exe",
    
    # Build time tooling dependencies
    "libclang.dll",
    
    # Test dependencies
    "VK_GPUOpen_Test_UserDataLayer.json",
    "vulkan-1.dll"
]

# All required files
require_list = [
    # <The> GPU Reshape
    "GPUReshape.exe",
]

# All required folders
require_folders = [
    "Plugins",
    "runtimes/win",
    "runtimes/win-x64",
    "runtimes/win-arm64",
    "runtimes/win7-x64"
]

# Get all packages
packages = sys.argv[1:]

# Info
sys.stdout.write(f"Creating packages for {packages}\n")

# Create all packages
for package in packages:
    sys.stdout.write(f"\nPackaging {package}\n")

    # Source directory
    bin_dir = os.path.join(bin_dir_root, package)
    
    # Destination directory
    pck_dir = os.path.join(pck_dir_root, package)

    # Remove old package data
    if os.path.exists(pck_dir):
        sys.stdout.write("\tRemoving old package...\n")
        shutil.rmtree(pck_dir)
       
    # Ensure tree exists
    os.makedirs(pck_dir)
    
    # Process all files
    for filename in os.listdir(bin_dir):
        src_path = os.path.join(bin_dir, filename)
    
        # Skip directories
        if os.path.isdir(src_path):
            continue

        # Filter    
        is_required = filename in require_list
        is_ignored  = any([x in filename for x in ignore_list])
        
        # Ignore?
        if not is_required and is_ignored:
            sys.stdout.write(f"\tIgnoring {filename}\n")
            continue

        # Copy!
        sys.stdout.write(f"\tPackaging {filename}\n")
        shutil.copyfile(src_path, os.path.join(pck_dir, filename))
        
    # Process all folders
    for folder in require_folders:
        bin_folder_dir = os.path.join(bin_dir, folder)
        pck_folder_dir = os.path.join(pck_dir, folder)
        
        # Remove old path
        if os.path.exists(pck_folder_dir):
            os.makedirs(pck_folder_dir)
            
        # Copy all contents
        sys.stdout.write(f"\tPackaging {folder}\n")
        shutil.copytree(bin_folder_dir, pck_folder_dir)
