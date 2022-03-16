# Shader Export Schema

Schema attributes for shader exports, extends the functionality of regular messages for IL generation, enabling GPU message creation. All capabilities and restrictions
of regular messages apply unless otherwise stated.

All shader exports are given the type `shader-export`:
```xml
<schema>
    <shader-export>
        ...
    </shader-export>
</schema>
```

Shader exports are compatible with standard message streams, 
however impose several restrictions for performance.

### Attributes

The following attributes are supported.

**no-sguid** </br>
Set the implicit source guid identifier mode, spanning 16 bits. 
If disabled, reverse source lookup will not be possible.

**structured** </br>
Sets the storage method. Unstructured exports guarantee the largest bandwidth, 
however limit the size to 32 bits. Structured exports have no size constraints at the cost of performance.

It is recommended to always tightly pack an export to 32 bits or less for optimal performance.

### Limitations

- Only static schema support (+)

[+ planned addition, - not planned, x not possible]