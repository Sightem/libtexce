# libtexce Autotests Makefile
# requires AUTOTESTER_ROM environment variable set to ROM path
#
# Usage:
#   make generate          # Generate test case directories
#   make build             # Build all tests (use -j for parallel)
#   make test-all          # Run all tests in parallel with summary
#   make test              # Run all tests sequentially with verbose output
#   make clean             # Clean all build artifacts

AUTOTESTER := $(shell which autotester 2>/dev/null || echo $(shell pwd)/../cemu/tests/autotester/autotester)

CASEGEN_DIR := casegen
CASEGEN_BIN := $(CASEGEN_DIR)/gen_cases
GENERATED_DIR := generated
GENERATED_CASES_MK := $(GENERATED_DIR)/cases.mk
EXPORT_IMAGES_BIN := export_images
CXX ?= c++

GENERATED_TESTS :=
-include $(GENERATED_CASES_MK)

TESTS := $(GENERATED_TESTS)

BUILD_TARGETS := $(addsuffix /.built,$(TESTS))

RESULTS_DIR := $(GENERATED_DIR)/.results

.PHONY: all clean build test test-all generate export-images

all: build

generate: $(CASEGEN_BIN)
	@./$(CASEGEN_BIN) --out $(GENERATED_DIR)

$(CASEGEN_BIN): $(CASEGEN_DIR)/generate_autotests.c $(CASEGEN_DIR)/cases.c $(CASEGEN_DIR)/cases.h
	@$(CC) -std=c23 -O2 -Wall -Wextra -o $(CASEGEN_BIN) $(CASEGEN_DIR)/generate_autotests.c $(CASEGEN_DIR)/cases.c

build: generate $(BUILD_TARGETS)

%/.built: generate
	@echo "Building $*"
	@$(MAKE) -C $* --no-print-directory all
	@touch $@

test-all:
	@if [ -z "$(AUTOTESTER_ROM)" ]; then \
		echo "Error: AUTOTESTER_ROM not set"; \
		echo "Usage: AUTOTESTER_ROM=/path/to/rom make test-all"; \
		exit 1; \
	fi
	@rm -rf $(RESULTS_DIR) && mkdir -p $(RESULTS_DIR)
	@echo "Running $(words $(TESTS)) tests in parallel..."
	@for dir in $(TESTS); do \
		( \
			CASE_NAME=$$(echo "$$dir" | tr '/' '_'); \
			if $(AUTOTESTER) $(AUTOTESTER_FLAGS) $$(pwd)/$$dir/autotest.json > /dev/null 2>&1; then \
				echo "  OK $$dir"; \
				touch $(RESULTS_DIR)/$$CASE_NAME.pass; \
			else \
				echo "  FAIL $$dir"; \
				touch $(RESULTS_DIR)/$$CASE_NAME.fail; \
			fi \
		) & \
	done; \
	wait
	@echo ""
	@PASSED=$$(ls $(RESULTS_DIR)/*.pass 2>/dev/null | wc -l); \
	FAILED=$$(ls $(RESULTS_DIR)/*.fail 2>/dev/null | wc -l); \
	echo "Summary: $$PASSED passed, $$FAILED failed"; \
	if [ "$$FAILED" -gt 0 ]; then \
		echo "Failed tests:"; \
		for f in $(RESULTS_DIR)/*.fail; do \
			[ -f "$$f" ] && basename "$$f" .fail | tr '_' '/'; \
		done; \
		exit 1; \
	fi

# sequential test
test: build
	@if [ -z "$(AUTOTESTER_ROM)" ]; then \
		echo "Error: AUTOTESTER_ROM not set"; \
		echo "Usage: AUTOTESTER_ROM=/path/to/rom make test"; \
		exit 1; \
	fi
	@PASSED=0; FAILED=0; \
	for dir in $(TESTS); do \
		echo "Testing $$dir"; \
		$(AUTOTESTER) $(AUTOTESTER_FLAGS) $$(pwd)/$$dir/autotest.json && PASSED=$$((PASSED+1)) || FAILED=$$((FAILED+1)); \
	done; \
	echo ""; \
	echo "Summary: $$PASSED passed, $$FAILED failed"

clean:
	@for dir in $(TESTS); do \
		$(MAKE) -C $$dir clean --no-print-directory 2>/dev/null || true; \
		rm -f $$dir/.built; \
	done
	@rm -rf $(RESULTS_DIR)
	@rm -f $(CASEGEN_BIN)
	@rm -f $(EXPORT_IMAGES_BIN)

export-images: export_images.cpp
	@$(CXX) -std=c++23 -O2 -Wall -Wextra -o $(EXPORT_IMAGES_BIN) export_images.cpp $(shell pkg-config --cflags --libs libpng freetype2 fontconfig) -lm
