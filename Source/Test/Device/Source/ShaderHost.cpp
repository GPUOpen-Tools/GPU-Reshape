#include <Test/Device/ShaderHost.h>

ShaderHost &ShaderHost::Get() {
    static ShaderHost host;
    return host;
}
