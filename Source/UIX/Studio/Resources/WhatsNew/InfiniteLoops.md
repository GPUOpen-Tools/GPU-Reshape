## Infinite Loops

The loop feature is no longer experimental with this release, and introduces new safe guards to catch potentially TDR causing loops. Instrumentation now injects both atomic checks, signaled from the host device/CPU, and in-shader iteration limits.

Any loop that either exceeds the iteration limit, or is signaled from the host, will break and report the faulting line of code.

![loop-source](avares://GPUReshape/Resources/WhatsNew/Images/loop-source.png)

The latter, in-shader iteration limits, is fully configurable with any number of maximum iterations. Applications may need to manually tune these numbers for their particular workload.

![loop-configuration](avares://GPUReshape/Resources/WhatsNew/Images/loop-configuration.png)

Please note that side effects in erroneous loop iterations, below the iteration limit, are not guarded beyond the existing feature set. For example, if the loop iterator is used to index a constant buffer array, the device may be lost regardless of loop instrumentation. Future features will try to address this.

