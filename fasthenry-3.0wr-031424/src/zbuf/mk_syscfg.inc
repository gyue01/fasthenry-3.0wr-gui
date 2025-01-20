# These CFLAGS should be fine for anything that is reasonably recent
# except for MinGW.

#CFLAGS = -O -g -DFOUR
CFLAGS = -O3 -DFOUR

# For MinGW, there is no getrusage, and we'll skip a gethostname call
# to avoid having to link with winsock.

#CFLAGS = -O -g -DFOUR -DNO_RUSAGE -DNO_GETHOSTNAME
#CFLAGS = -O3 -DFOUR -DNO_RUSAGE -DNO_GETHOSTNAME

# We're going to assume gcc, you'll need to comment this if a
# non-compatible compiler is used.
CC := gcc
