#pragma once

#ifndef ALIGN_TO
#   ifdef _MSC_VER
#       define ALIGN_TO(N) __declspec(align(4))
#   else
#       define ALIGN_TO(N) __attribute__((aligned(alignment)))
#   endif
#endif
