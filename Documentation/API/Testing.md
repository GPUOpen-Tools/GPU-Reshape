# Testing

A testing framework provides code generation for instrumentation unit testing, and message validation. The test generator accepts a single
.hlsl file with inlined annotations describing the test environment. From this, a full testing suite for all backends is generated, which tests for API stability
and the exported message contents against the attributes assigned.

Using this framework is optional, however is highly preferred to promote standardization and reduce the test LOC (of several magnitudes). 

---

Code quick start
- Implementation
  - [Generator](../../Source/Test/Device/Generator) </br>
- Examples
  - [Resource Bounds Unit Test](../../Source/Features/ResourceBounds/Backend/Tests/Data/SimpleTest.hlsl)

---

### Annotations

All annotations follow the `//!` annotation, any comments now following this will be ignored. The succeeding word defines the annotation type.

#### KERNEL

`//! KERNEL [TYPE] "[NAME]"`

The kernel annotation defines the **kernel type** and **kernel name**. The kernel name defines the function entry point of the shader. The kernel type must be one of the following:
- Compute

#### DISPATCH

`//! DISPATCH [X], [Y], [Z]`

The dispatch annotation defines execution mode of the kernel as dispatch, and the **dispatch dimensions**.

#### SCHEMA

`//! SCHEMA "[PATH]"`

The schema annotation defines the, optional, schema used for message testing. The path supplied must be the generated header.

#### RESOURCE

`//! RESOURCE [RESOURCE_TYPE]<[FORMAT]> (size:[SIZE])`

The resource annotation requests that a resource is generated and bound.

The resource type must be one of:
- Buffer
- RWBuffer
- Texture1D
- RWTexture1D
- Texture2D
- RWTexture2D
- Texture3D
- RWTexture3D

For valid resource formats, please see the [format enum](../../Source/Libraries/Backend/Include/Backend/IL/Format.h).

#### MESSAGE

`//! MESSAGE [NAME][[COUNT]] ([NAME]:[VALUE])*`

The message annotation adds a message unit test for the next source line. The name defines the message name which must be present in the schema specified, count defines the expected message count for the particular line.
Optionally, a set of attributes can be specified and tested against.

### Limitations

- Only compute supported (+)

[+ planned addition, - not planned, x not possible]
