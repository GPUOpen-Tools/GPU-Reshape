# Design

_Subject to change through development_

With the requirement of multiple backend APIs, the design must be able to represent the 
various features in a platform and API agnostic manner. The proof-of-concept implementation
was implemented entirely as a Vulkan layer, while this is not an accurate representation
of the improved design, it does provide insight into what capabilities a feature needs in order
to operate.

## 1. Features

Each feature operates in three fronts, the backend, analysis and frontend.

The backend is responsible for appropriate call hooking, instrumentation of shader code and
any additional state management CPU side. Note that not all features make use of shader instrumentation,
some validation may be performed entirely during call hooking. This alone is enough
for the majority of validation to take place.

Some features however require sufficient post processing / analysis of the produced validation data in only to report 
any meaningful data, such as branch coverage reports. The analysis stage is responsible for transforming
the messages produced by the backend into meaningful data.

Finally, the data is to be presented in an appropriate frontend. The exact nature does not matter, it may be a web page or desktop application,
but it is responsible for presenting the final validation data in a user friendly format.

This separation of behaviour also allows for the separation of data, meaning that the individual
stages do not need to execute on the same process or machine. Allowing for the latter stages to be executed remotely, separating the host and target machines, such as consoles.

### 1.1. Backend features

Each backend feature is responsible for hooking the relevant callbacks, and performing the needed
modifications / management for the feature. This is typically performing dynamic injecting of shader code
and potentially sending additional data to the device by, fx. appending new descriptor entries.

In the proof-of-concept, no abstraction was needed as it simply targeted a single API. However, we do not have this
luxury. This design proposes implementing a generic SSA representation, which is suitable for cross compilation
into the various backends. The backend features will perform hooking in a similar manner, however, will now operate on
said SSA form instead of SPIRV as in the proof-of-concept.

Additionally, the various API concepts will need appropriate abstraction and generalization, such as descriptor-set management. However, one must be careful to
not generalize to the extent at which you loose all the specialization that the various APIs offer these days. Additionally, it should
be possible to still hook API specific calls when required. The exact balance in the abstraction will remain to be seen, and will likely change as the features are iterated on.

### 1.2. Analysis features

The backend features intercept the messages produced by the backend features, and interpret them in a meaningful way. Most validation features
do not need extensive analysis of the produced messages, however certain features such as branch coverage reports need to.

As the analysis feature consumes messages, the filtered reports or the like are additional outgoing messages. Keeping the
ingoing and outgoing type the same, means that features which do not require analysis can be passed through when not consumed.

### 1.3. Frontend features

Finally, the frontend feature is responsible for creating a suitable representation of the incoming messages.
The exact design of this is highly dependent on the UI solution to be used, and as it is the last part of the pipeline, there is no need to impose strict restrictions.

