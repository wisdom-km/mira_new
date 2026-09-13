// AppState: Implementation for DirectorDesk App state (FND-10).
#include "DirectorDesk/App/AppState.h"

#include "AppInternals.h"
#include "DirectorDesk/App/ProjectFile.h"

namespace DirectorDesk::App {

AppState::AppState() : AppState(std::string()) {}

AppState::AppState(std::string officialCacheRoot)
    : registry(Asset::CreateDefaultRegistry())
    , officialCatalog(officialCacheRoot, MakeOfficialEndpoints())
    , officialCache(std::move(officialCacheRoot))
    , projectId(ProjectFile::MakeProjectId()) {}

} // namespace DirectorDesk::App
