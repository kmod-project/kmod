// SPDX-License-Identifier: LGPL-2.1-or-later

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <shared/tmpfile-util.h>
#include <shared/util.h>

static int get_random_from_dev(void *p, size_t n)
{
    int fd;
    ssize_t l;

    if (!p || n == 0)
        return -EINVAL;

    fd = open("/dev/urandom", O_RDONLY | O_NOCTTY);
    if (fd < 0) {
        return -EIO;
    }

    do {
        l = read(fd, p, n);
        if (l < 0) {
            if (errno == EINTR)
                continue;
            close(fd);
            return -errno;
        }

        if (l == 0) {
            close(fd);
            return -EIO;
        }

        p = (uint8_t *)p + l;
        n -= l;
    } while (n > 0);
    
    close(fd);
    return 0;
}

static int get_random_from_syscall(void *p, size_t n)
{
    static bool have_grndinsecure = true;
    ssize_t l;

    if (!p || n == 0)
        return -EINVAL;

    do {
        l = getrandom(p, n, have_grndinsecure ? GRND_INSECURE : GRND_NONBLOCK);
        if (l < 0) {
            if (errno == EINVAL && have_grndinsecure) {
                // No GRND_INSECURE; fallback to GRND_NONBLOCK.
                have_grndinsecure = false;
                continue;
            } else {
                // Will block (with GRND_NONBLOCK), or unexpected error.
                return -errno;
            }
        }

        if (l == 0) {
            return -errno;
        }

        p = (uint8_t *) p + l;
        n -= l;
    } while (n > 0);

    return 0;
}

#if 0
static int get_random_from_rand(void *p, size_t n)
{
    size_t i = 0;
    uint8_t *u8p = (uint8_t *)p;
    if (!p || n == 0)
        return -EINVAL;

    srand(time(0));

    for (i = 0; i < n; i ++) {
        u8p[i] = rand() % 0xFF;
    }
    return 0;
}
#endif

static int random_bytes(void *p, size_t n) {
    int err;

    err = get_random_from_syscall(p, n);
    if (err == 0)
        return 0;

    return get_random_from_dev(p, n);
}

static int __tempfn_random(const char *path, char **ret_file)
{
    _cleanup_free_ char *tmp_file = NULL;
    struct timeval tv;
    uint64_t rnd_u64;
    int err;
    char tmp[NAME_MAX] = "";
    int n;

	gettimeofday(&tv, NULL);

    err = random_bytes(&rnd_u64, sizeof(uint64_t));
    if (err < 0) {
        // Failed to generate random number, fall back to use timestamp to generate temporary name
        n = snprintf(tmp, sizeof(tmp), "%i.%lli.%lli", getpid(),
                (long long)tv.tv_usec, (long long)tv.tv_sec);
        if (n >= (int)sizeof(tmp)) {
            return -EINVAL;
        }

        err = asprintf(&tmp_file, "%s/%s", path, tmp);
        if (err < 0)
            return -ENOMEM;
    } else {
        err = asprintf(&tmp_file, "%s/%" PRIx64, path, rnd_u64);
        if (err < 0)
            return -ENOMEM;
    }

    if (ret_file)
        *ret_file = TAKE_PTR(tmp_file);

    return 0;
}

static int tempfn_random(const char *path, char **ret_file)
{
    _cleanup_free_ char *tmp_file = NULL;
    size_t try_count = 10; /* Try 10 times to generate a unique temporary file name */

    do {
        int err = __tempfn_random(path, &tmp_file);
        if (err < 0)
            continue;

        // Check if the file already exists
        if (access(tmp_file, F_OK) < 0) {
            if (errno == ENOENT) {
                if (ret_file)
                    *ret_file = TAKE_PTR(tmp_file);
                return 0;
            }
        }
    } while(--try_count > 0);

    return -EINVAL;
}

int fopen_temporary_at(int dir_fd, const char *path, int o_flags, int o_mode, int *ret_fd, char **ret_path) {
    _cleanup_free_ char *tmp_file = NULL;
    int fd = -EBADF;
    int r;

    if (dir_fd < 0 || !path)
        return -EINVAL;

    r = tempfn_random(path, &tmp_file);
    if (r < 0)
        return r;

    fd = openat(dir_fd, tmp_file, o_flags, o_mode);
    if (fd < 0) {
        (void) unlinkat(dir_fd, tmp_file, 0);
        return -errno;
    }

    *ret_fd = fd;
    if (ret_path)
        *ret_path = TAKE_PTR(tmp_file);

    return 0;
}
