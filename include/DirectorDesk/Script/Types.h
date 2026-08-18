#pragma once

#include <string>
#include <vector>

namespace DirectorDesk::Script {

enum class DiagnosticSeverity {
    Error,
    Warning,
    Hint,
};

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    int line = 1;
    std::string code;
    std::string message;
};

struct Shot {
    std::string id;
    std::string title;
    std::string body;
    int headingLine = 1;
};

struct Scene {
    std::string id;
    std::string title;
    std::string body;
    int headingLine = 1;
    std::vector<Shot> shots;
};

struct Snapshot {
    std::string documentTitle;
    std::vector<Scene> scenes;
};

struct ParseResult {
    bool completed = false;
    bool utf8Valid = true;
    Snapshot snapshot;
    std::vector<Diagnostic> diagnostics;
};

} // namespace DirectorDesk::Script
