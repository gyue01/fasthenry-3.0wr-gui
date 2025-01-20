
# This will create a Windows package in the current directory.  It needs
# the inno packager installed in /inno-5.5.9
# 
# You can tune mkwinpkg and/or fasthenry.iss.in to your needs.


package::
	files/mkwinpkg
	rm -rf data fasthenry.iss

clean::
	rm -rf data fasthenry.iss *.exe *.log

distclean::
	rm -rf data fasthenry.iss *.exe *.log
