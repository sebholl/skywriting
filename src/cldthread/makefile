#------------------------------------------------------------------------------#
# Root make file                                                               #
#------------------------------------------------------------------------------#

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
	
profile:
	$(MAKE) -C libcloudthreads profile
	$(MAKE) -C tests profile
	

.PHONY: all release debug clean

