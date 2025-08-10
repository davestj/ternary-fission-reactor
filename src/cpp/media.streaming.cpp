/*
 * File: src/cpp/media.streaming.cpp
 * Author: OpenAI Assistant
 * Date: August 10, 2025
 * Title: Media Streaming Manager Implementation
 * Purpose: Launches and controls ices2 subprocess for media streaming
 * Reason: Provides HTTP-controlled audio/video streaming capability
 *
 * Change Log:
 * 2025-08-10: Initial implementation
 */

#include "media.streaming.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <iostream>
#include <utility>

namespace TernaryFission {

MediaStreamingManager::MediaStreamingManager(std::string media_root, std::string icecast_mount)
    : media_root_(std::move(media_root)),
      icecast_mount_(std::move(icecast_mount)),
      streaming_pid_(-1),
      streaming_active_(false) {}

bool MediaStreamingManager::startStreaming() {
    std::lock_guard<std::mutex> lock(streaming_mutex_);
    if (streaming_active_) {
        return true;
    }

    pid_t pid = fork();
    if (pid == 0) {
        std::string playlist = media_root_ + "/playlist.m3u";
        execlp("ices2", "ices2", "-F", playlist.c_str(), "-m", icecast_mount_.c_str(), (char*)nullptr);
        _exit(1);
    } else if (pid > 0) {
        streaming_pid_ = pid;
        streaming_active_ = true;
        return true;
    } else {
        std::cerr << "Failed to fork ices2 process" << std::endl;
        return false;
    }
}

bool MediaStreamingManager::stopStreaming() {
    std::lock_guard<std::mutex> lock(streaming_mutex_);
    if (!streaming_active_) {
        return true;
    }

    if (kill(streaming_pid_, SIGTERM) == 0) {
        waitpid(streaming_pid_, nullptr, 0);
        streaming_active_ = false;
        streaming_pid_ = -1;
        return true;
    }

    std::cerr << "Failed to terminate ices2 process" << std::endl;
    return false;
}

bool MediaStreamingManager::isStreaming() const {
    return streaming_active_;
}

} // namespace TernaryFission
