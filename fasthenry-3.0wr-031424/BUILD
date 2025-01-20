
The build:

If you are building with MinGW, run "config mingw" from this directory.
This changes the CFLAGS defined in src/fasthenry/Makefile.  See the
comments in that file for more info. 

Type "make all" in this directory.  This will build the fasthenry and
zbuf programs, and install the binaries in ./bin.  You can move or
copy the binaries to another location, such as /usr/local/bin, by hand
if desired.

The old config and Makefiles are still around -- DON'T USE THEM except
for MinGW as above.  The present source files and default Makefiles
will work on any known reasonably modern system that is vaguely
Unix-like, including OS X, Cygwin, and of course Linux/*BSD.  If you
have trouble please report to stevew@wrcad.com.

The default is to build using the Sparse package included here, except
when building under the XicTools framework, where SuiteSparse/KLU is
the default.

To build using one of the alternative packages:
(MKL is not currently supported due to a licensing issue).
  1.  Install the alternative package (SuiteSparse or Intel MKL).
  2.  Modify the mk_klu.inc or mk_dss.inc files in src/fasthenry
      so that the paths correspond to your installation location.
  3.  Build by giving one of
      make SOLVER=KLU_SOLVER all
      make SOLVER=DSS_SOLVER all
      Note that you can make different versions, but you will need
      to give "make clean" before making a new version.

