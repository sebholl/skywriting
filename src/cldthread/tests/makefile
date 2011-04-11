#--------------------------------------------------------------------------------#
# Root test makefile - run ../updatemakefiles to sync with Code::Blocks projects #
#--------------------------------------------------------------------------------#

WRKDIR = `pwd`

all: release
	
release: release_build packages

debug: debug_build packages

release_build:
	$(MAKE) release -f fib.cbp.mak
	$(MAKE) release -f helloworld.cbp.mak
	$(MAKE) release -f linkedlist.cbp.mak
	$(MAKE) release -f pi.cbp.mak
	$(MAKE) release -f prodcons.cbp.mak
	$(MAKE) release -f mandelbrot.cbp.mak
	
debug_build:
	$(MAKE) debug -f fib.cbp.mak
	$(MAKE) debug -f helloworld.cbp.mak
	$(MAKE) debug -f linkedlist.cbp.mak
	$(MAKE) debug -f pi.cbp.mak
	$(MAKE) debug -f prodcons.cbp.mak
	$(MAKE) debug -f mandelbrot.cbp.mak
	
packages: clean_packages
	mkdir ./packages
	./createpacks.sh

clean: clean_packages
	$(MAKE) clean -f fib.cbp.mak
	$(MAKE) clean -f helloworld.cbp.mak
	$(MAKE) clean -f linkedlist.cbp.mak
	$(MAKE) clean -f pi.cbp.mak
	$(MAKE) clean -f prodcons.cbp.mak
	$(MAKE) clean -f mandelbrot.cbp.mak
	rm -f -r ./bin 
	rm -f *.layout *.depend

clean_packages:
	rm -f -r ./packages

.PHONY: all release debug clean

