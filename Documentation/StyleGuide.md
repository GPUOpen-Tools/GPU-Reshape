# Style Guide

_Coding style guide, incomplete_

## 1. C++

### 1.1. Casing

All type definitions, member definitions, function declarations and global definitions follow the Pascal Case. Each word within the name must be capitalized.

```c++
struct FooBar {
    void Foo();
    
    int NumFoos;
};
```

All local variables and function parameters follow the camel Case. First word must be in lowercase, followed by all remaining words capitalized.

```c++
struct FooBar {
    void Foo(int newFoos) {
        const int maxFoos = 16;
        
        NumFoos = std::min(maxFoos, newFoos);
    }
    
    int NumFoos;
};
```

### 1.2 Scope and Indentation

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
