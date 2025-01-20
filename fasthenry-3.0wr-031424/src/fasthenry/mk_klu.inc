#
# Linear Algebra extensions to FastHenry
# S. R. Whiteley 12/28/2018 wrcad.com
#

# KLU (SuiteSparse) solver defines.
SOLVER = KLU_SOLVER

# Change this to your XicTools KLU build area.  If you don't have the
# XicTools source, set this to your SuiteSparse build area.  In the
# latter case, you will either have to configure SuiteSparse to
# generate static libraries, or change the SOLVERLIB below to link
# dynamically.
#
KLU_HOME = $$HOME/src/xt/KLU

# CentOS 6 and others may need this, otherwise leave RTLIB commented.
#RTLIB = -lrt

SOLVERLIB = $(KLU_HOME)/KLU/Lib/libklu.a \
  $(KLU_HOME)/AMD/Lib/libamd.a \
  $(KLU_HOME)/COLAMD/Lib/libcolamd.a \
  $(KLU_HOME)/BTF/Lib/libbtf.a \
  $(KLU_HOME)/SuiteSparse_config/SuiteSparse_config.o $(RTLIB)

SOLVERINC = -I$(KLU_HOME)/KLU/Include -I$(KLU_HOME)/AMD/Include \
 -I$(KLU_HOME)/COLAMD/Include -I$(KLU_HOME)/BTF/Include \
 -I$(KLU_HOME)/SuiteSparse_config

