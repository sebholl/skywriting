#------------------------------------------------------------------------------#
# Directory make file - forwards most commands onto libcloudthreads.cbp.mak    #
#------------------------------------------------------------------------------#

WRKDIR = `pwd`
MAKE = make

all:
	$(MAKE) virtual_all -f libcloudthreads.cbp.mak
	
release:
	$(MAKE) release -f libcloudthreads.cbp.mak
	
debug:
	$(MAKE) debug -f libcloudthreads.cbp.mak

clean:
	$(MAKE) clean -f libcloudthreads.cbp.mak
	rm -f -r 'bin'
	rm -f -r 'doc'
	rm -f -r '.obj'
	rm -f *.layout *.depend
	
doxygen:
	$(MAKE) doxygen -f libcloudthreads.cbp.mak
	
profile:
	$(MAKE) profile -f libcloudthreads.cbp.mak
	

.PHONY: all release debug clean

