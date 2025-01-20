# These CFLAGS should be fine for anything that is reasonably recent
# except for MinGW.

# Check AVX (parallel math support), for x86_64 only.  We're going to
# assume the compiler is gcc compatible, you'll need to comment this
# if a non-compatible compiler is used.  Allow AVX if the gcc version
# is 4.8 or later.  This should also work for macOS clang on Intel
# Macs.

ifeq "$(shell uname -m)" "x86_64"
HAVX = 0
CC := gcc
GCC_major := "`gcc -dumpversion | cut -f1 -d.`"
GCC_minor := "`gcc -dumpversion | cut -f2 -d.`"
CHKMAJG := $(shell expr $(GCC_major) ">" 4)
CHKMAJE := $(shell expr $(GCC_major) "=" 4)
ifeq "$(CHKMAJG)" "1"
    HAVX = 1
else
ifeq "$(CHKMAJE)" "1"
CHKMINGE := $(shell expr $(GCC_minor) ">=" 8)
ifeq "$(CHKMINGE)" "1"
    HAVX = 1
endif
endif
endif
endif

# If you build with AVX enabled, and run on a system that does not
# have AVX, the program will crash.  If is not likely that you would
# want to run fasthenry on such a machine as it is probably slow, but
# to force build without AVX comment the next three lines below. 
# There seems to be no way to accommodate the presence/absence of AVX
# at run-time nicely.

ifeq "$(HAVX)" "1"
    MAVX = -mavx -DWITH_AVX
endif

CFLAGS = -O3 -DFOUR $(MAVX) 

# For debugging.
#CFLAGS = -O -g -DFOUR $(MAVX)

# For profiling.
#CFLAGS = -pg -g  -DFOUR $(MAVX)

