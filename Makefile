.PHONY: run build format release test package

run:
	LSAN_OPTIONS=suppressions=lsan.supp ./debug/bin/flatearth_testbed

build:
	./build-all.sh && ./post-build.sh

format:
	@find engine/src testbed/src tests/src \( -name "*.cc" -o -name "*.hpp" \) -print0 | xargs -0 clang-format -i
	@echo "make format: All source and header files formatted accordingly to .clang-format file"

release:
	./build-all.sh release && ./post-build.sh release

test:
	./build-all.sh && ./debug/bin/flatearth_tests

# Produces flatearth-sdk-v<VERSION>-linux.tar.gz containing:
#   lib/  — libflatearth.so + versioned symlinks
#   include/ — all public engine headers (engine/src/**/*.hpp)
package:
	@echo "==> Building engine (release only)..."
	cd engine && ./run.sh release
	$(eval FE_VERSION := $(shell grep -oP 'LIBRARY_VERSION_STRING \K[0-9.]+' engine/CMakeLists.txt | head -1))
	$(eval SDK_DIR    := flatearth-sdk-v$(FE_VERSION))
	$(eval TARBALL    := $(SDK_DIR)-linux.tar.gz)
	$(eval LIB_SRC    := engine/build-release/libflatearth.so.$(FE_VERSION))
	@echo "==> Packaging SDK v$(FE_VERSION)..."
	rm -rf $(SDK_DIR)
	mkdir -p $(SDK_DIR)/lib $(SDK_DIR)/include
	cp $(LIB_SRC) $(SDK_DIR)/lib/
	cd $(SDK_DIR)/lib && \
	  ln -sf libflatearth.so.$(FE_VERSION) libflatearth.so.0 && \
	  ln -sf libflatearth.so.0 libflatearth.so
	find engine/src -name "*.hpp" | while read f; do \
	  rel=$$(echo "$$f" | sed 's|engine/src/||'); \
	  mkdir -p "$(SDK_DIR)/include/$$(dirname $$rel)"; \
	  cp "$$f" "$(SDK_DIR)/include/$$rel"; \
	done
	tar -czf $(TARBALL) $(SDK_DIR)/
	rm -rf $(SDK_DIR)
	@echo "==> Created $(TARBALL)"
