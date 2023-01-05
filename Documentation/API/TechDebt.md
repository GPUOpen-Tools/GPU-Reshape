# Tech Debt

_Keeping track of tech debt_

## Systematic

- Unify resource identifiers for various systems, several systems refer to the same thing with different identifiers, confusing capabilities and adds needless redundancy.
- Error handling is a mess, assertions on creation failures is not the way. Introduce proper error handling.

## Improvements

- Crack down on micro allocations, particularly during instrumentation.
- Add support for polymorphic allocators, own ALL allocations.

## Cleanliness
- The current state of namespaces are a mess, settle on a guide that is friendly for integration.
