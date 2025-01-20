#
# Linear Algebra extensions to FastHenry
# S. R. Whiteley 12/28/2018 wrcad.com
#

# Edit this file to link to the MKL/DSS library on your system.  One can
# use static or dynamic linking.  If dynamic linking, you probably need
# to add the path in $(SOLVER_LIB) to your LD_LIBRARY_PATH.

# The Intel MKL/DSS solver.
SOLVER = DSS_SOLVER

### Mac setup
#DSS_HOME = /opt/intel/compilers_and_libraries/mac/mkl
#SOLVER_LIB = $(DSS_HOME)/lib

### Generic Linux setup
DSS_HOME = /opt/intel/compilers_and_libraries/linux/mkl
SOLVER_LIB = $(DSS_HOME)/lib/intel64

### common
SOLVERINC = -I$(DSS_HOME)/include

#  Static link
#SOLVERLIB = -Wl,-Bstatic -L$(SOLVER_LIB) \
#  -lmkl_intel_lp64 \
#  -lmkl_sequential \
#  -lmkl_core \
#  -lmkl_intel_lp64 \
#  -lmkl_sequential \
#  -lmkl_core \
#  -lmkl_intel_lp64 \
#  -lmkl_sequential \
#  -lmkl_core \
#  -Wl,-Bdynamic -lpthread -lm -lrpcsvc -lc -ldl 

#  Dynamic link
SOLVERLIB = -L$(SOLVER_LIB) \
  -lmkl_intel_lp64 \
  -lmkl_sequential \
  -lmkl_core \
  -lpthread -lm -lrpcsvc -lc -ldl 
