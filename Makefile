.PHONY: all
all: debug

.PHONY: debug
debug:
	cd bin/debug && ./marsengen && gprof ./marsengen -b > profile.gprof
