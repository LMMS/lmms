# Required by cmake if `uname -s` is inadaquate
SET(CMAKE_SYSTEM_NAME               Windows)
SET(CMAKE_SYSTEM_VERSION            1)

# Search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# For libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)