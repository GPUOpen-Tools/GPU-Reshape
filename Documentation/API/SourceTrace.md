# Source Trace

Source traces provide efficient source backtracing from validation errors, at the cost of increased validation message sizes.
All validation messages, unless attributed with `no-sguid`, automatically contain the source traceback information with a fixed memory cost of 16 bits.

Instructions of interest that may need source backtracing request a shader source GUID allocation, unique for the representative source mapping for the 
instruction. Reverse lookup converts the GUID back into source mappings.

---

Code quick start

- Examples
    - [Resource Bounds Injection](../../Source/Features/ResourceBounds/Backend/Source/Feature.cpp)

---

### Limitations

- Maximum of 65'536 simultaneously active source mappings (-)

[+ planned addition, - not planned, x not possible]
