#pragma once

int fopen_temporary_at(int dir_fd, const char *path, int o_flags, int o_mode, int *ret_fd, char **ret_path);
