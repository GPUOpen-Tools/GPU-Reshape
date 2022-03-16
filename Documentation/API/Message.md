# Messages

Messages are the atomic unit of communication when data needs to be transferred between two places.
They serve cross-process, machine and hardware unit communication, and as such must be extremely light in memory usage and serialization / deserialization overhead.

Messages are defined using schemas, an XML representation of the messages contents used for code generation. For accepted grammar, see:

- [Message Grammar](Schema/Message.md)
- [Shader Export Grammar](Schema/ShaderExport.md)

---

Code quick start

- Implementation
    - [Library](../../Source/Libraries/Message) </br>
    - [Generator](../../Source/Libraries/Message/Generator) </br>
    - [Shader Export Extension](../../Source/Libraries/Backend/Generator)
- Examples
    - [Logging](../../Source/Libraries/Message/Schemas/Log.xml)
    - [Resource Bounds](../../Source/Features/ResourceBounds/Backend/Schemas/ResourceBounds.xml)

---

### Streams

A stream serves as a contiguous container for a number of messages. Streams are appendable and iterable from front to back.
While a stream may contain any number of messages, it may be restricted by the **stream schema** set.

#### Schema

A stream schema is a set of restrictions for a given stream. There exists currently three stream schemas:

1. Static - The message stream only contains a single message type with a fixed stride
2. Dynamic - The message stream only contains a single type, however allows for variable strides. Use cases for this may be dynamic arrays (fx. strings).
3. Ordered - The message stream can contain any type of any length, this has the largest cost not only in iteration, but also memory usage.

Additionally, the stream schema defines the **header** of all messages. The header is pre-pended to all messages, which has large implications for memory usage.

- Static schemas have no header as iteration and insertion requires no dynamic information.
- Dynamic schemas require a 4-byte header with the size of the message.
- Ordered schemas require an 8-byte header with the type and size of the message.

For small messages in performance critical paths, it is highly recommended to always use the static message schema.

### Inline memory layout

The dynamic and ordered schemas follow an inline memory layout model in which all indirections is contained within the memory region of the message.
All dynamic containers within a message, such as arrays, only host relative offsets to the container. This means that there is no need for
serialization or deserialization.

Allocation of messages with dynamic contents require up front allocation info, describing the size of any dynamic field.

### Generators

The message generator, responsible for parsing and performing code generation on the XML, supports extensions to extend the
code generation capabilities. This is to avoida monolith generator for all project uses, which would violate a forward dependency tree.

One of such use cases is the **shader-export** message type, which generates automatic message exporting on the GPU. 

### Languages

Currently only C++ is supported as a target language generator, however C# support is planned for GUI development.
