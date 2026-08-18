// main: Implementation for the DirectorDesk App module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/App/Application.h"

int main(int argc, char** argv) {
    DirectorDesk::App::Application application;
    return application.Run(argc, argv);
}
