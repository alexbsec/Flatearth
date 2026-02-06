.PHONY: run build format release

run:
	LSAN_OPTIONS=suppressions=lsan.supp ./debug/bin/flatearth_testbed

build:
	./build-all.sh && ./post-build.sh

format:
	@find . \( -name "*.cc" -o -name "*.hpp" \) -print0 | xargs -0 clang-format -i
	@echo "make format: All source and header files formatted accordingly to .clang-format file"

release:
	./build-all.sh release && ./post-build.sh release
