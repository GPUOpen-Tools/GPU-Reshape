#pragma once

#ifndef ALIGN_TO
#   ifdef _MSC_VER
#       define ALIGN_TO(N) __declspec(align(4))
#   else
#       define ALIGN_TO(N) __attribute__((aligned(alignment)))
#   endif
#endif

#ifndef ALIGN_PACK
#   ifdef _MSC_VER
#       define ALIGN_PACK
#   else
#       define ALIGN_PACK __attribute__ ((packed))
#   endif
#endif
