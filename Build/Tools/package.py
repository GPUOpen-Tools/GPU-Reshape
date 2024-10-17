# 
# The MIT License (MIT)
# 
# Copyright (c) 2024 Advanced Micro Devices, Inc.,
# Fatalist Development AB (Avalanche Studio Group),
# and Miguel Petersen.
# 
# All Rights Reserved.
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
import subprocess

# Paths
root_dir     = os.path.join("../", "../")
bin_dir_root = os.path.join(root_dir, "Bin")
pck_dir_root = os.path.join(root_dir, "Package")

# Ignore list
ignore_list = [
    # Executables, handled through required files
    # Mostly unit testing, build time tooling
    ".exe",
    
    # All configuration json, handled through required files
    ".json",
    
    # Build time tooling dependencies
    "libclang.dll",
    
    # Test dependencies
    "GRS.Libraries.Message.Benchmark.DotNet.dll",
    "GRS.Backends.Vulkan.Tests.UserDataLayer.dll",
    "vulkan-1.dll"
]

publish_projects = [
    # "Source/UIX/Studio/Studio.csproj",
    # "Source/Services/Discovery/DotNet/NotifyIcon/NotifyIcon.csproj"
]

# All required files
require_list = [
    # <The> GPU Reshape
    "GPUReshape.exe",
    
    # Executable dependencies
    "GPUReshape.NotifyIcon.exe",                 # Task bar notification icon
    "GRS.Backends.DX12.Service.exe",             # Handles project detouring for D3D12 objects
    "GRS.Backends.DX12.Service.RelFunTBL.exe",   # Relative function table generator for x86
    "GRS.Services.HostResolver.Standalone.exe",  # Host resolver tool for discovery
    "GRS.Services.Discovery.Cleanup.exe",        # General cleanup tool for discovery
    "XamlColorSchemeGenerator.exe",              # UIX dependency
    
    # UIX runtime dependency json
    "GPUReshape.deps.json",
    "GPUReshape.runtimeconfig.json",
    "GPUReshape.NotifyIcon.deps.json",
    "GPUReshape.NotifyIcon.runtimeconfig.json",
    "VK_LAYER_GPUOPEN_GRS.json"
]

# All required folders
require_folders = [
    "Plugins",
    "Dependencies"
]

# All extra folders
extra_folders = [
    ("Documentation/Redist", "Redist")
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
    sym_dir = os.path.join(pck_dir_root, package + "Sym")

    # Remove old package data
    if os.path.exists(pck_dir):
        sys.stdout.write("\tRemoving old package...\n")
        shutil.rmtree(pck_dir)

    # Remove old package symbol data
    if os.path.exists(sym_dir):
        sys.stdout.write("\tRemoving old package symbols...\n")
        shutil.rmtree(sym_dir)
       
    # Ensure tree exists
    os.makedirs(pck_dir)
    os.makedirs(sym_dir)
    
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
            
        # Destination folder
        dest_dir = sym_dir if ".pdb" in filename else pck_dir

        # Copy!
        sys.stdout.write(f"\tPackaging {filename}\n")
        shutil.copyfile(src_path, os.path.join(dest_dir, filename))
        
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
        
    # Process all extra folders
    for folder in extra_folders:
        src_folder_dir = os.path.join(root_dir, folder[0])
        pck_folder_dir = os.path.join(pck_dir,  folder[1])
        
        # Remove old path
        if os.path.exists(pck_folder_dir):
            os.makedirs(pck_folder_dir)
            
        # Copy all contents
        sys.stdout.write(f"\tPackaging {folder[0]}\n")
        shutil.copytree(src_folder_dir, pck_folder_dir)
        
    # Package all relevant projects
    for project in publish_projects:
        subprocess.run([
            "dotnet",
            "publish",
            os.path.join(root_dir, project),
            "-c", package.split('/')[-1],
            "-o", pck_dir,
            "--self-contained",
            "-r", "win-x64"
        ])
