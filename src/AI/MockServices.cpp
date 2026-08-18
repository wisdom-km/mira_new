#include "DirectorDesk/AI/MockServices.h"

namespace DirectorDesk::AI {
namespace {

Core::Error Invalid(const std::string& technical, const std::string& user) {
    return Core::Error::Make(Core::ErrorCode::InvalidArgument, technical, user);
}

Core::Error MissingJob() {
    return Core::Error::Make(Core::ErrorCode::NotFound, "unknown gen job", "找不到该生成任务");
}

Core::Error NotReady() {
    return Core::Error::Make(Core::ErrorCode::NotFound, "gen job not finished", "任务尚未完成");
}

bool ReferencesSafe(const std::vector<ReferenceImage>& references) {
    for (const ReferenceImage& image : references) {
        if (!IsSafeReferencePath(image.path)) {
            return false;
        }
    }
    return true;
}

bool ShotValid(const ShotGenContext& shot) {
    return !shot.shotId.empty();
}

} // namespace

void MockImageGenService::FailNext(Core::Error error) {
    m_failNext = true;
    m_nextError = std::move(error);
}

void MockImageGenService::Pump() {
    for (auto& entry : m_jobs) {
        Job& job = entry.second;
        if (job.progress.status == GenJobStatus::Queued) {
            job.progress.status = GenJobStatus::Running;
            job.progress.ratio = 0.5f;
            job.progress.message = "running";
            continue;
        }
        if (job.progress.status != GenJobStatus::Running) {
            continue;
        }
        if (job.fail) {
            job.progress.status = GenJobStatus::Failed;
            job.progress.error = job.failError;
            job.progress.message = job.failError.userMessage;
            continue;
        }
        job.progress.status = GenJobStatus::Succeeded;
        job.progress.ratio = 1.0f;
        job.progress.message = "succeeded";
        job.result.jobId = job.progress.jobId;
        job.result.outputPath = "mock-output/" + job.progress.jobId + ".png";
        job.result.width = job.request.width;
        job.result.height = job.request.height;
        job.result.shot = job.request.shot;
        job.result.references = job.request.references;
    }
}

Core::Result<std::string> MockImageGenService::Submit(const ImageGenRequest& request) {
    if (!ShotValid(request.shot)) {
        return Core::Result<std::string>::Fail(
            Invalid("missing shotId", "生成请求缺少镜头 ID"));
    }
    if (request.width == 0 || request.height == 0) {
        return Core::Result<std::string>::Fail(
            Invalid("invalid image size", "生成尺寸无效"));
    }
    if (!ReferencesSafe(request.references)) {
        return Core::Result<std::string>::Fail(
            Invalid("remote reference path", "参考图只能使用本地路径"));
    }
    Job job;
    job.request = request;
    job.progress.jobId = "img-" + std::to_string(++m_nextId);
    job.progress.status = GenJobStatus::Queued;
    job.progress.message = "queued";
    if (m_failNext) {
        job.fail = true;
        job.failError = m_nextError;
        m_failNext = false;
        m_nextError = {};
    }
    const std::string id = job.progress.jobId;
    m_jobs.emplace(id, std::move(job));
    return Core::Result<std::string>::Ok(id);
}

Core::Result<void> MockImageGenService::Cancel(const std::string& jobId) {
    auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        return Core::Result<void>::Fail(MissingJob());
    }
    if (IsTerminal(it->second.progress.status)) {
        return Core::Result<void>::Fail(
            Invalid("job already finished", "任务已结束，无法取消"));
    }
    it->second.progress.status = GenJobStatus::Cancelled;
    it->second.progress.message = "cancelled";
    it->second.progress.error =
        Core::Error::Make(Core::ErrorCode::Cancelled, "cancelled", "已取消生成");
    return Core::Result<void>::Ok();
}

Core::Result<GenProgress> MockImageGenService::Progress(const std::string& jobId) const {
    const auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        return Core::Result<GenProgress>::Fail(MissingJob());
    }
    return Core::Result<GenProgress>::Ok(it->second.progress);
}

Core::Result<ImageGenResult> MockImageGenService::TakeResult(const std::string& jobId) {
    const auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        return Core::Result<ImageGenResult>::Fail(MissingJob());
    }
    if (it->second.progress.status == GenJobStatus::Failed) {
        const Core::Error error = it->second.progress.error;
        m_jobs.erase(it);
        return Core::Result<ImageGenResult>::Fail(error);
    }
    if (it->second.progress.status == GenJobStatus::Cancelled) {
        const Core::Error error = it->second.progress.error;
        m_jobs.erase(it);
        return Core::Result<ImageGenResult>::Fail(error);
    }
    if (it->second.progress.status != GenJobStatus::Succeeded) {
        return Core::Result<ImageGenResult>::Fail(NotReady());
    }
    ImageGenResult result = it->second.result;
    m_jobs.erase(it);
    return Core::Result<ImageGenResult>::Ok(std::move(result));
}

void MockVideoGenService::FailNext(Core::Error error) {
    m_failNext = true;
    m_nextError = std::move(error);
}

void MockVideoGenService::Pump() {
    for (auto& entry : m_jobs) {
        Job& job = entry.second;
        if (job.progress.status == GenJobStatus::Queued) {
            job.progress.status = GenJobStatus::Running;
            job.progress.ratio = 0.5f;
            job.progress.message = "running";
            continue;
        }
        if (job.progress.status != GenJobStatus::Running) {
            continue;
        }
        if (job.fail) {
            job.progress.status = GenJobStatus::Failed;
            job.progress.error = job.failError;
            job.progress.message = job.failError.userMessage;
            continue;
        }
        job.progress.status = GenJobStatus::Succeeded;
        job.progress.ratio = 1.0f;
        job.progress.message = "succeeded";
        job.result.jobId = job.progress.jobId;
        job.result.outputPath = "mock-output/" + job.progress.jobId + ".mp4";
        job.result.width = job.request.width;
        job.result.height = job.request.height;
        job.result.durationMs = job.request.durationMs;
        job.result.frameRate = job.request.frameRate;
        job.result.shot = job.request.shot;
        job.result.references = job.request.references;
    }
}

Core::Result<std::string> MockVideoGenService::Submit(const VideoGenRequest& request) {
    if (!ShotValid(request.shot)) {
        return Core::Result<std::string>::Fail(
            Invalid("missing shotId", "生成请求缺少镜头 ID"));
    }
    if (request.width == 0 || request.height == 0 || request.durationMs == 0 ||
        request.frameRate == 0) {
        return Core::Result<std::string>::Fail(
            Invalid("invalid video params", "视频生成参数无效"));
    }
    if (!ReferencesSafe(request.references)) {
        return Core::Result<std::string>::Fail(
            Invalid("remote reference path", "参考图只能使用本地路径"));
    }
    Job job;
    job.request = request;
    job.progress.jobId = "vid-" + std::to_string(++m_nextId);
    job.progress.status = GenJobStatus::Queued;
    job.progress.message = "queued";
    if (m_failNext) {
        job.fail = true;
        job.failError = m_nextError;
        m_failNext = false;
        m_nextError = {};
    }
    const std::string id = job.progress.jobId;
    m_jobs.emplace(id, std::move(job));
    return Core::Result<std::string>::Ok(id);
}

Core::Result<void> MockVideoGenService::Cancel(const std::string& jobId) {
    auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        return Core::Result<void>::Fail(MissingJob());
    }
    if (IsTerminal(it->second.progress.status)) {
        return Core::Result<void>::Fail(
            Invalid("job already finished", "任务已结束，无法取消"));
    }
    it->second.progress.status = GenJobStatus::Cancelled;
    it->second.progress.message = "cancelled";
    it->second.progress.error =
        Core::Error::Make(Core::ErrorCode::Cancelled, "cancelled", "已取消生成");
    return Core::Result<void>::Ok();
}

Core::Result<GenProgress> MockVideoGenService::Progress(const std::string& jobId) const {
    const auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        return Core::Result<GenProgress>::Fail(MissingJob());
    }
    return Core::Result<GenProgress>::Ok(it->second.progress);
}

Core::Result<VideoGenResult> MockVideoGenService::TakeResult(const std::string& jobId) {
    const auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        return Core::Result<VideoGenResult>::Fail(MissingJob());
    }
    if (it->second.progress.status == GenJobStatus::Failed) {
        const Core::Error error = it->second.progress.error;
        m_jobs.erase(it);
        return Core::Result<VideoGenResult>::Fail(error);
    }
    if (it->second.progress.status == GenJobStatus::Cancelled) {
        const Core::Error error = it->second.progress.error;
        m_jobs.erase(it);
        return Core::Result<VideoGenResult>::Fail(error);
    }
    if (it->second.progress.status != GenJobStatus::Succeeded) {
        return Core::Result<VideoGenResult>::Fail(NotReady());
    }
    VideoGenResult result = it->second.result;
    m_jobs.erase(it);
    return Core::Result<VideoGenResult>::Ok(std::move(result));
}

} // namespace DirectorDesk::AI
