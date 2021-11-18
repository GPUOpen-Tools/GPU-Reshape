# Tech Debt

_Keeping track of tech debt_


#### Namespaces

Pollution of global namespaces, will make it hard to integrate this library into existing codebases. Create a style guide for scoping

#### Std Maps

Investigate alternatives to std maps due to their iterable storage constraints. (add allocator support to the poly maps)

### Instrumentation

- The job submission is a little allocation heavy, can be much more clever