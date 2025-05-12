#!/bin/bash
/test/fasthenry /test/tmp.inp
cat /Zc.mat
/test/fasthenry -f simple /test/tmp.inp
/test/zbuf /zbuffile
ps2pdf /zbuffile.ps
pdftoppm -png /zbuffile.pdf zbuffile
tail -f /dev/null

