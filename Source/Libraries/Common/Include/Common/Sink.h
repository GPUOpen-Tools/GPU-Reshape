#pragma once

namespace Detail {
    template<typename... T>
    void Sink(T&&...) {}
}

#ifndef GRS_SINK
#   define GRS_SINK(...) while(false) { ::Detail::Sink(__VA_ARGS__); } (void)0
#endif // GRS_SINK
