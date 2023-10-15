# 
# The MIT License (MIT)
# 
# Copyright (c) 2023 Miguel Petersen
# Copyright (c) 2023 Advanced Micro Devices, Inc
# Copyright (c) 2023 Fatalist Development AB
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

import glob
import os
import sys
import chardet
from datetime import datetime

# ------------- #
# Configuration #
# ------------- #

# Starting year of the license
base_year = 2023

# All search directories
directories = [
    "../../Build/**/*.*",
    "../../Source/**/*.*",
    "../../ThirdParty/*.*",
    "../../*.*"
]

# Ignore paths
ignore = [
    "/venv/",
    "/obj/",
    "/bin/",
    "/.idea/"
]

# All accepted file names
extensions = (".h", ".cpp", ".hpp", ".inl", ".cs", ".hlsl", ".cmake", ".py", ".bat", ".axaml", ".xml")

# All special filenames
specials = {
    "CMakeLists.txt": ".cmake"
}

# Alternate commenting styles
mappings = {
    "default": ("// ", "// ", ""),
    ".cmake": ("# ", "# ", ""),
    ".py": ("# ", "# ", ""),
    ".bat": (":: ", ":: ", ""),
    ".axaml": ("<!--\n", "  ", "\n-->"),
    ".xml": ("<!--\n", "  ", "\n-->")
}

# ---------- #
# Generation #
# ---------- #

# Get current year
current_year = datetime.now().year

# Determine range
license_range = base_year if base_year == current_year else f"{base_year} - {current_year}"

# Expected header, also used for validity testing
license_header = "The MIT License (MIT)"

# License template
license_contents = f"""
{license_header}

Copyright (c) {license_range} Miguel Petersen
Copyright (c) {license_range} Advanced Micro Devices, Inc
Copyright (c) {license_range} Fatalist Development AB

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
"""

# CLRF by default
license_contents = license_contents.replace("\n", "\r\n")

# All mapping templates
license_templates = {}

# Expand mappings
for ext in extensions:
    comment = mappings[ext] if ext in mappings else mappings["default"]
    license_templates[ext] = comment[0] + license_contents.replace("\n", "\n" + comment[1]) + comment[2] + "\r\n\r\n"


# Compare two strings, ignoring line ending differences
def starts_with_no_le(a, b):
    alen = len(a)
    blen = len(b)

    # Iteration offsets
    aoffset = 0
    boffset = 0

    # While both offsets are in bounds
    while aoffset < alen and boffset < blen:
        # Skip CRLF
        if a[aoffset] == '\r':
            aoffset += 1
            continue

        # Skip CRLF
        if b[boffset] == '\r':
            boffset += 1
            continue

        # Check character
        if a[aoffset] != b[boffset]:
            return False
        
        # Next!
        aoffset += 1
        boffset += 1

    # At end?
    return boffset == blen


# Diagnostics
n_processed_count = 0

# Process all files
for directory in directories:
    for file in glob.glob(directory, recursive=True):
        # Part of extension set? And not a special file?
        ext      = os.path.splitext(file)[1]
        basename = os.path.basename(file)
        if ext not in extensions and basename not in specials:
            continue

        # Ignored?
        if any(ign in file.replace("\\", "/") for ign in ignore):
            continue

        # New extension type for license
        if basename in specials:
            ext = specials[basename]

        # Determine mapping
        comment = mappings[ext] if ext in mappings else mappings["default"]

        # Given template
        template = license_templates[ext]

        # Diagnostic
        sys.stdout.write(f"Updating '{file}' ...")

        # Copy contents
        with open(file, 'rb') as f:
            bin = f.read()
            encoding = chardet.detect(bin)['encoding']
            encoding = encoding if encoding is not None else 'ascii'
            contents = bin.decode(encoding)

        # Find first non-comment
        end = 0
        while any(len(x) > 0 and contents.startswith(x, end) for x in comment):
            end = contents.index("\n", end) + 1

        # Eat last newline
        end = contents.index("\n", end) + 1

        # Perfectly licensed?
        if starts_with_no_le(contents, template):
            sys.stdout.write(f" Up to date.\n")
            continue

        # If not a license, do not stomp
        if contents.find(license_header, 0, end) == -1:
            end = 0

        # License the file
        contents = template + contents[end:]

        # Write contents
        with open(file, 'wb') as f:
            f.write(contents.encode(encoding))

        # OK
        sys.stdout.write(f" Updated as {encoding}!\n")
        n_processed_count += 1

# Separate
sys.stdout.write("\n")

# Diagnostic
if n_processed_count > 0:
    sys.stdout.write(f"Updated {n_processed_count} files\n")
else:
    sys.stdout.write(f"No files updated\n")
