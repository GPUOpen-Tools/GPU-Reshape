#pragma once

namespace Detail {
    template<typename... T>
    void Sink(T&&...) {}
}

#ifndef GRS_SINK
#   define GRS_SINK(...) { ::Detail::Sink(__VA_ARGS__); } while(false) (void)0
#endif // GRS_SINK
