
# Flags for MSYS2/MINGW.

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

# For MinGW, there is no getrusage, and we'll skip a gethostname call
# to avoid having to link with winsock.

# The AVX intrinsics reputedly can cause seg faults in MINGW due to
# inability to stack align to 32-bits.  However, this may have been
# fixed.  To try your luck and enable AVX, uncomment the CFLAGS line
# that contains MAVX, and comment the line that doesn't.
#
#CFLAGS = -O3 -DFOUR -DNO_RUSAGE -DNO_GETHOSTNAME $(MAVX)
CFLAGS = -O3 -DFOUR -DNO_RUSAGE -DNO_GETHOSTNAME

# For debugging.
#CFLAGS = -O -g -DFOUR -DNO_RUSAGE -DNO_GETHOSTNAME
