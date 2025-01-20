# These CFLAGS should be fine for anything that is reasonably recent
# except for MinGW.

# We're going to assume gcc, you'll need to comment this if a
# non-compatible compiler is used.  Turn on SIMD if gcc version is 4.8
# or later.
CC := gcc
GCC_major := "`gcc -dumpversion | cut -f1 -d.`"
GCC_minor := "`gcc -dumpversion | cut -f2 -d.`"
CHKMAJG := $(shell expr $(GCC_major) ">" 4)
CHKMAJE := $(shell expr $(GCC_major) "=" 4)
ifeq "$(CHKMAJG)" "1"
    MAVX = -mavx -DWITH_AVX
else
ifeq "$(CHKMAJE)" "1"
CHKMINGE := $(shell expr $(GCC_minor) ">=" 8)
ifeq "$(CHKMINGE)" "1"
    MAVX = -mavx -DWITH_AVX
endif
endif
endif

CFLAGS = -O3 -DFOUR $(MAVX) 

# For debugging.
#CFLAGS = -O -g -DFOUR $(MAVX)

# For profiling.
#CFLAGS = -pg -g  -DFOUR $(MAVX)

