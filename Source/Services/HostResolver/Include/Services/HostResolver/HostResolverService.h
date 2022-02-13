#pragma once

class HostResolverService {
public:
    /// Install this service
    /// \return success state
    bool Install();

private:
    /// Start the host resolve process
    /// \return success state
    bool StartProcess();
};
