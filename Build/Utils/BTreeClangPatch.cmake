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

# Clang only has partial support for the relaxed typename rules

# Expected path
set(BTreeHPath "${BTREE_PATH}/fc_btree.h")

# Replace relaxed typename instances
file(READ ${BTreeHPath} Contents)
string(REPLACE "using PairRefType = TreePairRef<TreePair>::type;" "using PairRefType = typename TreePairRef<TreePair>::type;" Contents "${Contents}")
string(REPLACE "using difference_type = IterTraits::difference_type;" "using difference_type = typename IterTraits::difference_type;" Contents "${Contents}")
string(REPLACE "using value_type = IterTraits::value_type;" "using value_type = typename IterTraits::value_type;" Contents "${Contents}")
string(REPLACE "using pointer = IterTraits::pointer;" "using pointer = typename IterTraits::pointer;" Contents "${Contents}")
string(REPLACE "using reference = IterTraits::reference;" "using reference = typename IterTraits::reference;" Contents "${Contents}")
string(REPLACE "using iterator_category = IterTraits::iterator_category;" "using iterator_category = typename IterTraits::iterator_category;" Contents "${Contents}")
string(REPLACE "using iterator_concept = IterTraits::iterator_concept;" "using iterator_concept = typename IterTraits::iterator_concept;" Contents "${Contents}")
file(WRITE ${BTreeHPath} "${Contents}")
