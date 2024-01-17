# Features

Status of all implemented and planned features.

## 1. Feature Support

Scopes mark the intended development cycle in which the feature is planned to be implemented, **scope at release only includes V0**.

| Feature                      | Type         | Proof-of-concept   | Implemented        | Scope | 
|------------------------------|--------------|--------------------|--------------------|-------|
| Resource bounds              | Validation   | :white_check_mark: | :white_check_mark: | V0    |
| Descriptor bounds            | Validation   | :white_check_mark: | :white_check_mark: | V0    |
| Resource export stability    | Validation   | :white_check_mark: | :white_check_mark: | V0    |
| Concurrency                  | Validation   | :white_check_mark: | :white_check_mark: | V0    |
| Resource initialization      | Validation   | :white_check_mark: | :white_check_mark: | V0    |
| Waterfall detection          | Performance  |                    | :atom_symbol:      | P0    |
| Bank conflict detection      | Performance  |                    |                    | P0    |
| Numeric & builtin stability  | Validation   |                    |                    | V1    |
| Indirect parameters          | Validation   |                    |                    | V1    |
| Direct Parameters            | Validation   |                    |                    | V1    |
| Branch termination           | Validation   |                    |                    | V1    |
| Branch coverage              | Optimization |                    |                    | O0    |
| Branch coherence             | Optimization |                    |                    | O0    |
| Branch hot spots             | Optimization |                    |                    | O0    |
| Scalarization potential      | Optimization |                    |                    | O0    |
| Shader assertions            | Debugging    |                    |                    | D0    |
| Shader instruction debugging | Debugging    |                    |                    | D0    |
| Shader live editing          | Debugging    |                    |                    | D0    |

( :white_check_mark: - completed, :atom_symbol: - in development )

### 2. Investigation

| Feature                     | Type         | Implemented |
|-----------------------------|--------------|-------------|
| Access patterns / coherence | Optimization |             |
| Profile guided optimization | Optimization |             |
