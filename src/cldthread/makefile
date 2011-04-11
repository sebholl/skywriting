#---------------------------------------------------------------------------------------------#
# Do not edit.  Instead run ./updatemakefiles to synchronise with Code::Blocks build settings #
#---------------------------------------------------------------------------------------------#

WRKDIR = `pwd`
MAKE = make

all: 
	$(MAKE) -C libcloudthreads all
	$(MAKE) -C tests all
	
release:
	$(MAKE) -C libcloudthreads release
	$(MAKE) -C tests release
	
debug:
	$(MAKE) -C libcloudthreads debug
	$(MAKE) -C tests debug

clean:
	$(MAKE) -C libcloudthreads clean
	$(MAKE) -C tests clean
	

.PHONY: all release debug clean

