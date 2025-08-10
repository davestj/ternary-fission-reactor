<!--
File: docs/media.streaming.md
Author: OpenAI Assistant
Date: August 10, 2025
Title: Media Streaming Setup Guide
Purpose: Instructions for preparing playlists and using ices2 with the daemon
Reason: Document new audio/video streaming capabilities
Change Log:
- 2025-08-10: Initial media streaming documentation
-->

# Media Streaming Setup

The daemon can optionally stream audio or video content to an Icecast server using the `ices2` utility. Enable streaming by setting the following configuration fields:

```
media_streaming_enabled = true
media_root = /path/to/media
icecast_mount = /stream
```

## Preparing Playlists

1. Place media files (e.g., `.mp3`, `.ogg`, `.webm`) under the directory specified by `media_root`.
2. Create a `playlist.m3u` file in that directory listing the media files to stream, one path per line:
   ```
   song1.mp3
   song2.ogg
   video1.webm
   ```

## Starting `ices2`

The server launches `ices2` as a subprocess when `/api/v1/stream/start` is called. Ensure `ices2` is installed and accessible in `PATH`.

The process is invoked with:
```
ices2 -F <media_root>/playlist.m3u -m <icecast_mount>
```
This reads the playlist and streams the entries to the configured Icecast mount point.

## Manual Control

Use the HTTP endpoints to control streaming:

```bash
curl -X POST http://localhost:8333/api/v1/stream/start
curl -X POST http://localhost:8333/api/v1/stream/stop
```

If `ices2` exits or fails to start, the endpoints return an error.

## Notes

- The Icecast server must be running and configured to accept streams on the given mount.
- Supported formats include `audio/mpeg`, `audio/ogg`, and `video/webm`.
- Update `playlist.m3u` at runtime to change streamed content.
