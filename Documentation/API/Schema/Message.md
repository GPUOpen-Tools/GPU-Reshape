# Message Schema

All messages schema must define the root as `schema`, from which a set of messages and related fields can be defined.

```xml
<schema>
    ...
</schema>
```

### Messages

All messages, unless provided by an extension, must have the type `message`.

```xml
<schema>
    <message name="messageName">
        ...
    </message>
</schema>
```

#### Attributes

The following attributes are supported.

**name** </br>
Sets the name of the message, generated message class will be named `[name]Message`.

### Fields

Fields are members to the parent message type.

```xml
<schema>
    <message>
        <field name="fieldName" type="fieldType"/>
        ...
    </message>
</schema>
```

#### Attributes

The following attributes are supported:

**name** </br>
Sets the generated name of the field.

**type** </br>
Sets the type of the field, see **types** section for support and additional attributes available.

**value** </br>
Sets the default value of this element.

### Types

#### Primitive

The following primitive types are supported:
- uint64
- uint32
- uint16
- uint8
- int64
- int32
- int16
- int8
- float
- double
- bool

Primitive types extend the attribute support with the following:

**bits** </br>
Sets the number of bits used by the primitive type. Neighboring primitives of the same type with this attribute are packed together, when possible.

#### Arrays

Array types `array` requires the `element` attribute to be provided, which defines the contained
element type.

#### Strings

The string type `string` enables a variable width string.

#### Sub-Streams

Streams may host sub streams `stream`, which may be of another schema type than the containing stream. This is particularly useful for 
specialization of dynamic behaviour. It is highly recommended to avoid this in performance critical paths.
