.DEFAULT_GOAL := all

MKDIR = mkdir -p


all:
	${MKDIR} bin
	$(MAKE) -C client
	$(MAKE) -C server
	$(MAKE) -C test

debug:
	${MKDIR} bin
	$(MAKE) debug -C client
	$(MAKE) debug -C server
	$(MAKE) -C test

clean:
	$(MAKE) clean -C client
	$(MAKE) clean -C server
	$(MAKE) clean -C test
