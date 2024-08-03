
## Let's reinvent the wheel, again

Unfortunately the previous reactive event bindings are unsupported for Avalonia 11. As a replacement, or recommendation,
the Pharmacist (MsBuild) package is recommended. However, this appears to have moved to ReactiveMarbles.ObservableEvents,
which is a source generator.

After evaluating the build and IDE (since it invokes the generator) performance, I instead decided to just wrap them
manually. 
