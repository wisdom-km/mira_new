#include "DirectorDesk/AI/Types.h"

namespace DirectorDesk::AI {

const char* JobStatusId(GenJobStatus status) {
    switch (status) {
    case GenJobStatus::Queued:
        return "queued";
    case GenJobStatus::Running:
        return "running";
    case GenJobStatus::Succeeded:
        return "succeeded";
    case GenJobStatus::Failed:
        return "failed";
    case GenJobStatus::Cancelled:
        return "cancelled";
    }
    return "queued";
}

bool IsTerminal(GenJobStatus status) {
    return status == GenJobStatus::Succeeded || status == GenJobStatus::Failed ||
           status == GenJobStatus::Cancelled;
}

bool IsSafeReferencePath(const std::string& path) {
    if (path.empty()) {
        return true;
    }
    return path.find("://") == std::string::npos;
}

} // namespace DirectorDesk::AI
