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
