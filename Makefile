.PHONY: run

run:
	LSAN_OPTIONS=suppressions=lsan.supp ./bin/flatearth_testbed


.PHONY: build

build:
	./build-all.sh && ./post-build.sh
