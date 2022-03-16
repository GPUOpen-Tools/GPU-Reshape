# Tech Debt

_Keeping track of tech debt_


#### Namespaces

- Pollution of global namespaces, will make it hard to integrate this library into existing codebases. Create a style guide for scoping

#### Containers

- Add support for polymorphic allocators for the standard containers
- Fall back to stack allocators when possible to avoid allocations

#### Instrumentation

- The job submission is a little allocation heavy, can be much more clever
- Move instrumentation controller to shared code
