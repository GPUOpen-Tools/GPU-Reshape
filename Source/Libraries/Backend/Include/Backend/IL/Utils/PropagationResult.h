#pragma once

namespace Backend::IL {
    enum class PropagationResult {
        /// No mapped propagation
        None,

        /// Propagation is ignored, i.e. no interesting values can be propagated
        Ignore,

        /// Propagation is mapped with a compile time constant
        Mapped,

        /// Propagation is known varying
        Varying
    };
}
