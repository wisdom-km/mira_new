// Application: Public or internal interface for the DirectorDesk App module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

namespace DirectorDesk::App {

class Application {
public:
    int Run(int argc = 0, char** argv = nullptr);
};

} // namespace DirectorDesk::App
