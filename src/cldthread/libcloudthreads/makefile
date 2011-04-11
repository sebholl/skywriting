#--------------------------------------------------------------------------------#
# Root test makefile - run ../updatemakefiles to sync with Code::Blocks project  #
#--------------------------------------------------------------------------------#

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
	rmdir 'bin'
	rm -f -r 'doc'
	rm -f *.layout *.depend
	

.PHONY: all release debug clean

