#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#else
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#endif