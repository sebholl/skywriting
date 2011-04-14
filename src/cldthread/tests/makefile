#------------------------------------------------------------------------------#
# Directory make file (which calls respective cbp2make generated scripts)      #
#------------------------------------------------------------------------------#

WRKDIR = `pwd`

all: release
	
release:
	$(MAKE) release -f fib.cbp.mak
	$(MAKE) release -f helloworld.cbp.mak
	$(MAKE) release -f linkedlist.cbp.mak
	$(MAKE) release -f pi.cbp.mak
	$(MAKE) release -f prodcons.cbp.mak
	$(MAKE) release -f mandelbrot.cbp.mak
	./updatesymlinks.sh Release
	
debug:
	$(MAKE) debug -f fib.cbp.mak
	$(MAKE) debug -f helloworld.cbp.mak
	$(MAKE) debug -f linkedlist.cbp.mak
	$(MAKE) debug -f pi.cbp.mak
	$(MAKE) debug -f prodcons.cbp.mak
	$(MAKE) debug -f mandelbrot.cbp.mak
	./updatesymlinks.sh Debug

clean:
	$(MAKE) clean -f fib.cbp.mak
	$(MAKE) clean -f helloworld.cbp.mak
	$(MAKE) clean -f linkedlist.cbp.mak
	$(MAKE) clean -f pi.cbp.mak
	$(MAKE) clean -f prodcons.cbp.mak
	$(MAKE) clean -f mandelbrot.cbp.mak
	rm -f -r ./bin 
	rm -f -r ./.obj
	rm -f *.layout *.depend
	

.PHONY: all release debug clean

