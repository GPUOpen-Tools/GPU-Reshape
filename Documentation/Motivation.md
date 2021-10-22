# Motivation

Throughout the development of a game one often encounters numerous graphical issues, while many are approachable and easily detected there remains
two which consistently challenging.
- Visual Instability \
Visual corruption caused by both defined and potentially undefined operations, such as numeric instability due to insufficient precision.
- Device Instability \
Device (, OS) hangs & crashes caused by potentially undefined operations, such as accessing beyond the buffers subresource byte range. While
previous APIs attempted the safeguard against such usage newer APIs often do not, particularly Vulkan.
  
Previously in order to avoid any of the above a strong sense of awareness was required when using any potentially undefined operations GPU side,
however with the modern features available and the targeted hardware, this has become infeasible. Validating CPU side issues is less of an issue due to the wide availability of
instrumentation tools, however GPU side instrumentation remains to be seen. While this does directly translate to job
security it also means the waste of valuable time tracking down issues with practically no indication of where it originated from.

This leads us to the motivation behind this feature, a layer which attempts to validate potentially undefined behaviour and analysis of performance varying
operations on the GPU. While the primary purposes is the validation of such behaviour, it's secondary purpose is the analysis of operations which may
result in performance variations of shader invocations. One of such analysis may be the instrumentation of branching in order to produce a global branch
coverage report, this is extremely useful for performance analysis on any target / API.
