#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstdlib>
#include <cstdint>
#ifdef __APPLE__
#include <libproc.h>
#endif

static uint64_t get_fd_count() {
#ifdef __linux__
    DIR* fd_dir = opendir("/proc/self/fd");
    if (!fd_dir) return 0;
    uint64_t count = 0;
    struct dirent* entry;
    while ((entry = readdir(fd_dir)) != nullptr) {
        if (entry->d_name[0] != '.') count++;
    }
    closedir(fd_dir);
    return count;
#elif defined(__APPLE__)
    pid_t pid = getpid();
    int buf_size = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, nullptr, 0);
    if (buf_size <= 0) return 0;
    struct proc_fdinfo* fdinfo = static_cast<struct proc_fdinfo*>(malloc(buf_size));
    if (!fdinfo) return 0;
    int bytes = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, fdinfo, buf_size);
    uint64_t count = 0;
    if (bytes > 0) count = static_cast<uint64_t>(bytes) / PROC_PIDLISTFD_SIZE;
    free(fdinfo);
    return count;
#else
    return 0;
#endif
}

int main() {
    uint64_t initial = get_fd_count();
    int fd = open("/dev/null", O_RDONLY);
    assert(fd >= 0);
    uint64_t after_open = get_fd_count();
    assert(after_open == initial + 1);
    close(fd);
    uint64_t after_close = get_fd_count();
    assert(after_close == initial);
    return 0;
}
