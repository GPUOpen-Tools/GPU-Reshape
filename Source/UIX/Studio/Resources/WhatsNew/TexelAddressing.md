## Texel Addressing

Initialization and concurrency instrumentation have undergone major improvements in this release, now validating reads and writes on a per-texel level for textures, and per-byte for buffers. This greatly improves accuracy and avoids false-positives with concurrent usage of a single resource across multiple dispatches.

Missing initialization and race conditions will now report the exact coordinate that faulted. For example, if a compute shader clearing a full-screen texture does not clear the last row (e.g., due to faulty dispatching logic), future reads only on said row will fault.

![texel-addressing-il](avares://GPUReshape/Resources/WhatsNew/Images/texel-addressing-il.png)

Previously initialization and concurrency validation was tracked on a "per-resource" level, in which, for example, resources are marked as initialized on the first texel/byte write. This means that missing writes to specific regions are not detected.

Additionally, if concurrency is not tracked on the smallest granularity the hardware may write to, there is no reliable way to detect race conditions.

Texel addressing imposes large memory and runtime performance overhead, and will significantly increase instrumentation times (for initialization and concurrency validation). To fall back to per-resource tracking, disable "Texel addressing" in the launch window or application settings (under Features).

![texel-addressing-launch](avares://GPUReshape/Resources/WhatsNew/Images/texel-addressing-launch.png)

Please note that per-resource addressing is less accurate, and may report false positives on concurrent usage across multiple dispatches.
