/*
 * File: include/media.streaming.h
 * Author: OpenAI Assistant
 * Date: August 10, 2025
 * Title: Media Streaming Management for Ternary Fission Server
 * Purpose: Controls external media streaming tool invocation and lifecycle
 * Reason: Enables optional audio/video streaming via Icecast and ices2
 *
 * Change Log:
 * 2025-08-10: Initial implementation of media streaming manager
 */
#ifndef MEDIA_STREAMING_H
#define MEDIA_STREAMING_H

#include <string>
#include <mutex>
#include <sys/types.h>

namespace TernaryFission {

class MediaStreamingManager {
private:
    std::string media_root_;
    std::string icecast_mount_;
    pid_t streaming_pid_;
    bool streaming_active_;
    mutable std::mutex streaming_mutex_;

public:
    MediaStreamingManager(std::string media_root, std::string icecast_mount);
    bool startStreaming();
    bool stopStreaming();
    bool isStreaming() const;
};

} // namespace TernaryFission

#endif // MEDIA_STREAMING_H
