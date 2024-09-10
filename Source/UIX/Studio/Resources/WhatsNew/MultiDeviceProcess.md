## Multi Device/Process

GPU Reshape now supports hooking multiple devices and entire process trees. Previously, launches would only connect to the first reporting device, by checking "Capture All Devices" it now hooks and creates a workspace for every device upon creation. It also supports hooking entire process trees, meaning the target process and all of its child processes, should "Attach Child Processes" be enabled.

![launch-multi-device](avares://GPUReshape/Resources/WhatsNew/Images/launch-multi-device.png)

The latter is particularly helpful for applications such as WebGPU (e.g., Chrome) and bootstrapped launches.

![launch-workspaces](avares://GPUReshape/Resources/WhatsNew/Images/launch-workspaces.png)
