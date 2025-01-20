# Makefile for FastHenry

include mk_syscfg.inc

SHELL = /bin/sh
RM = /bin/rm
BIN = ../../bin

###############################################################################
# Linear algebra package configuration.

SOLVER := SPARSE_SOLVER
ifeq ($(SOLVER),SPARSE_SOLVER)

# Original "Sparse" package from Berkeley (modified here).
SOLVER = SPARSE_SOLVER
SOLVERLIB = ../sparse/sparse.a

else ifeq ($(SOLVER),KLU_SOLVER)
# KLU (SuiteSparse).
include mk_klu.inc

else ifeq ($(SOLVER),DSS_SOLVER)
# Intel MKL/DSS.
include mk_dss.inc

endif

#CFLAGS += -Wall

#### End of linear algebra package configuration.
###############################################################################

OBJS = induct.o gmres.o savemat_mod.o readGeom.o joelself.o writefastcap.o \
       SetupMulti.o calcp.o SetupComputePsi.o mulSetup.o BreakupSeg.o \
       Precond.o addgroundplane.o findpaths.o fillM.o cx_ludecomp.o \
       parse_command_line.o default_opts.o Prec_cost.o hole.o \
       dist_betw_fils.o mutual.o newPrecond.o deg_mutual.o \
       barnoldi.o regurgitate.o solver.o resusage.o

MOBJS = mulGlobal.o mulDo.o mulMulti.o mulLocal.o mulMats.o direct.o \
        fh_alloc.o capsolve.o 

NONUNIOBJS = find_nonuni_path.o read_tree.o contact.o

HEADER = induct.h cmplx.h resusage.h
MULHEAD = mulStruct.h mulGlobal.h patran.h resusage.h
NONUNIHEAD = gp.h

fasthenry::
	$(MAKE) $(BIN)/fasthenry
$(BIN)/fasthenry: $(OBJS) $(MOBJS) $(NONUNIOBJS)
	@if [ $(SOLVER) = SPARSE_SOLVER ]; then \
	    cd ../sparse; make CC=$(CC); \
	fi
	$(CC) -o $(BIN)/fasthenry $(CFLAGS) $(OBJS) $(MOBJS) $(NONUNIOBJS) \
$(SOLVERLIB) -lm

clean:
	$(RM) -f *.o
	cd ../sparse; make clean

solver.o: solver.c
	$(CC) $(CFLAGS) -D$(SOLVER) $(SOLVERINC) -c -o solver.o solver.c

$(OBJS):	$(HEADER) $(MULHEAD)

$(MOBJS):	$(MULHEAD)

$(NONUNIOBJS):  $(HEADER) $(MULHEAD) $(NONUNIHEAD)
