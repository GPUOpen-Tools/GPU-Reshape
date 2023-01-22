# Style Guide

_Coding style guide, incomplete_

## C++

### Casing

All type definitions, function declarations and global definitions follow the Pascal Case. Each word within the name must be capitalized.

```c++
int Global;

struct FooBar {
    void Foo();
};
```

All members variables, local variables and function parameters follow the camel Case. First word must be in lowercase, followed by all remaining words capitalized.

```c++
struct FooBar {
    void Foo(int newFoos) {
        const int maxFoos = 16;
        
        numFoos = std::min(maxFoos, newFoos);
    }
    
    int numFoos;
};
```

### Scope and Indentation

All tabs must be 4 spaces.

```cpp
a;
    a;
        a;
```

Scopes must start on the same line, except for in statement-block scopes.

```cpp
namespace A {
    
}

void Foo() {
    // Scope inside statement block
    {
        
    }
}
```

### Preprocessors

All preprocessors must be upper case with underscores for separation, and start with `GRS_`.

```cpp
#define GRS_DECLARE_TYPE
```

## C#

Please refer to the Microsoft C# code style.
