##### Configuration ###############################################################################

# Control message logging
# LOG_LEVEL may be any of 1: error, 2: warning, 3: info, 4: debug.
# All levels output messages from inferior levels.
FLAGS_LOG=-D_ENABLE_LOG_MESSAGES -D_LOG_LEVEL=3
#FLAGS_LOG=

# Control whether assert does anything at all.
FLAGS_DEBUG=
#FLAGS_DEBUG=-DNDEBUG

# Common settings
# Flags, -Wtraditional
FLAG_WARNINGS=-pedantic -Wall -Wextra -Wshadow -Wpointer-arith \
	-Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wconversion -Wno-overlength-strings

COMMON_CFLAGS=-std=c99 -D_POSIX_SOURCE=1 -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -g3 \
		-D_O_TMPFILE $(FLAG_WARNINGS) $(FLAGS_LOG) $(FLAGS_DEBUG)
COMMON_LDFLAGS=-g3

# A set of coverage-safe optimization flags (from http://onlinelibrary.wiley.com/doi/10.1002/stvr.1485/pdf)
COV_SAFE_OPT=-falign-functions -falign-jumps -falign-labels -falign-loops -fcprop-registers -fdefer-pop \
	-fforward-propagate -fguess-branch-probability -fif-conversion -fif-conversion2 \
	-fipa-pure-const -fipa-reference -fmerge-all-constants -fmerge-constants -fomit-frame-pointer -fsplit-wide-types \
	-ftree-ccp -ftree-copyrename -ftree-copy-prop -ftree-dce -ftree-dse -ftree-fre -ftree-sink -ftree-sra -ftree-ter \
	-fcaller-saves -fcrossjumping -fcse-follow-jumps -fipa-sra -fexpensive-optimizations -fgcse -fipa-cp \
	-foptimize-register-move -foptimize-sibling-calls -fpeephole2 -fregmove -freorder-blocks -freorder-functions \
	-frerun-cse-after-loop -fschedule-insns -fschedule-insns2 -ftree-switch-conversion -ftree-pre -ftree-vrp \
	-ftree-builtin-call-dce -fgcse-after-reload -fpredictive-commoning -ftree-vectorize
	
# Use these when building for coverage measurements
OPT_CFLAGS=$(COV_SAFE_OPT)

# Performance build
#OPT_CFLAGS=-O3 -flto

# Performance build without asserts (use these settings to prevent source code leaks).
#OPT_CFLAGS=-O3 -DNDEBUG -flto

OPT_LDFLAGS=$(OPT_CFLAGS)

# There are 4 different builds of this software

# (1) Main build (the software).
# This build produces all binaries in build/bin and the static library build/obj/main/*.a

CFLAGS=$(COMMON_CFLAGS) $(OPT_CFLAGS) -fvisibility=hidden -fdiagnostics-color=auto 
LDFLAGS=$(COMMON_LDFLAGS) $(OPT_LDFLAGS)

# (2) Test build (unit tests): Same flags + hardening + coverage instrumentation.
# This build produces the build/unittest binary and the build unittest.report document.

TEST_CFLAGS=$(COMMON_CFLAGS) $(OPT_CFLAGS) -D_GNU_SOURCE --coverage -fsanitize=undefined \
	-fno-sanitize-recover=all -D_GLIBC_DEBUG
TEST_LDFLAGS=$(COMMON_LDFLAGS) $(OPT_LDFLAGS) --coverage -fsanitize=undefined -fno-sanitize-recover=all \
	-D_GLIBC_DEBUG -lm
TEST_LDLIBS=

# (3) Fuzzing build: Binaries are instrumented for fuzzing.
# This build produces one binary in build/fuzzers/ for each .c file in fuzzing/.
# The binary is instrumeted for fuzzing with AFL. For example:
# afl-fuzz -i fuzzing/header_samples/ -o build/fuzzing_output/header_fuzzer build/fuzzers/header_fuzzer

FUZZ_CC=afl-clang-fast
FUZZ_CFLAGS=$(COMMON_CFLAGS) -D_GNU_SOURCE -fsanitize=undefined -fno-sanitize-recover=all \
	-Wno-gnu-statement-expression -g0
FUZZ_LDFLAGS=$(COMMON_LDFLAGS) -fsanitize=undefined -fno-sanitize-recover=all -g0
FUZZ_LDLIBS=

# (4) Fuzzing coverage: Binaries are instrumented for coverage reporting 
# One binary, this time with a name ending in _coverage, is produced in build/fuzzers/ for each .c file in fuzzing/.
# The binary is instrumeted for coverage testing to measure the actual coverage obtained by fuzzing.
# See fuzzing_coverage_queue and fuzzing_coverage_samples.

FUZZ_COV_CFLAGS=$(COMMON_CFLAGS) -D_GNU_SOURCE --coverage -O0 $(COV_SAFE_OPT)
FUZZ_COV_LDFLAGS=$(COMMON_LDFLAGS) --coverage -O0 $(COV_SAFE_OPT)

###### End of configuration ########################################################################

# To set all files to be secondary to speed up rebuilds we need proper dependency tracking (gcc -MM ...)
.SECONDARY: $(USAGE_FILES)

# Phony
.PHONY: bin lib test fuzzers docs clean scf

# Source folders

vpath %.md docs
vpath %.c bin src test fuzzing
vpath %.h bin src test fuzzing

# Source files

SRC_C_FILES=$(notdir $(wildcard src/*.c))
SRC_H_FILES=$(notdir $(wildcard src/*.h))
BIN_C_FILES=$(notdir $(wildcard bin/*.c))
BIN_H_FILES=$(notdir $(wildcard bin/*.h))
TEST_C_FILES=$(notdir $(wildcard test/*.c))
TEST_H_FILES=$(notdir $(wildcard test/*.h))
FUZZING_C_FILES=$(notdir $(wildcard fuzzing/*.c))
FUZZING_H_FILES=$(notdir $(wildcard fuzzing/*.h))
DOC_MD_FILES=$(notdir $(wildcard docs/*.md))

# Output files (prefixed)

DOC_PDF_FILES=$(addprefix build/pdf/, $(DOC_MD_FILES:.md=.pdf)) build/pdf/requirements_traceability_matrix.pdf


DOXYGEN_FILES=build/doxygen/html/index.html

# (1)
SRC_O_FILES=$(addprefix build/obj/main/, $(SRC_C_FILES:.c=.o))
BIN_O_FILES=$(addprefix build/obj/main/, $(BIN_C_FILES:.c=.o))

MAIN_BINARIES=$(addprefix build/bin/, $(filter-out bin_common, $(BIN_C_FILES:.c=)))

# (2)
TEST_O_FILES=$(addprefix build/obj/test/, $(TEST_C_FILES:.c=.o) $(SRC_C_FILES:.c=.o))
# (3)

FUZZING_O_FILES=$(addprefix build/obj/fuzzing/, $(SRC_C_FILES:.c=.o)) \
	build/obj/fuzzing/fuzzing_common.o \
	build/obj/fuzzing/bin_common.o \
	build/obj/fuzzing/command_line_fuzzer_common.o

FUZZING_COV_O_FILES=$(addprefix build/obj/fuzzing_coverage/, $(SRC_C_FILES:.c=.o)) \
	build/obj/fuzzing_coverage/fuzzing_common.o \
	build/obj/fuzzing_coverage/bin_common.o \
	build/obj/fuzzing_coverage/command_line_fuzzer_common.o

# (4)
FUZZER_BINARIES=$(addprefix build/fuzzers/, $(filter-out %_common, $(FUZZING_C_FILES:.c=))) \
	$(addprefix build/fuzzers/, $(filter-out %_common_coverage, $(FUZZING_C_FILES:.c=_coverage)))

# Main rules


all: .renamed bin fuzzers build/unittest test test_data docs
	@./metasrc/check_line_length.py
	@./metasrc/generate_scf.py

fast: .renamed bin fuzzers build/unittest

.renamed:
	echo "renaming..."
	./metasrc/rename_project.sh

# Run make full to make full documentation
fulldoc: all test_data scf coverage_report

code: bin build/unittest fuzzers
	./metasrc/check_line_length.py

# SCF
scf:
	python3 ./metasrc/generate_scf.py

# Test data

test_data: testdata/.generated

testdata/.generated: ./metasrc/generate_test_samples.py bin
	./metasrc/generate_test_samples.py -v

# Documentation 

docs: $(DOC_PDF_FILES) $(DOXYGEN_FILES) build/doxygen/latex/refman.pdf
	@cp build/doxygen/latex/refman.pdf build/pdf/design_document.pdf

build/doxygen/latex/refman.pdf: $(DOXYGEN_FILES)
	$(MAKE) -s -C build/doxygen/latex > /dev/null

build/doxygen/html/%.html: $(SRC_C_FILES) $(SRC_H_FILES) $(BIN_C_FILES) $(BIN_H_FILES) $(TEST_C_FILES) $(TEST_H_FILES)
	doxygen docs/conf/doxygen.conf > /dev/null

MARKDOWN_FORMAT_P1=auto_identifiers+pipe_tables+auto_identifiers+ascii_identifiers+autolink_bare_uris
MARKDOWN_FORMAT_P2=shortcut_reference_links+lists_without_preceding_blankline+yaml_metadata_block+pipe_tables
MARKDOWN_FORMAT=markdown+$(MARKDOWN_FORMAT_P1)+$(MARKDOWN_FORMAT_P2)

build/pdf/requirements_traceability_matrix.pdf: metasrc/build_rtm.py $(TEST_C_FILES) $(FUZZING_C_FILES) \
		$(wildcard crossvalidation/*.sh)
	@mkdir -p build/pdf
	metasrc/build_rtm.py > build/requirements_traceability_matrix.md
	pandoc build/requirements_traceability_matrix.md --template docs/conf/template.pandoc -f $(MARKDOWN_FORMAT) \
		-t latex -s --highlight-style=pygments -o $@ > /dev/null

build/pdf/%.pdf: %.md
	@mkdir -p build/pdf
	pandoc $^ --template docs/conf/template.pandoc -f $(MARKDOWN_FORMAT) -t latex -s --highlight-style=pygments -o $@ > /dev/null

# Binaries

BIN_FILES_WITH_USAGE=$(filter-out bin/bin_common.c, $(wildcard bin/*.c))
USAGE_FILES=$(BIN_FILES_WITH_USAGE:.c=_usage.h)

bin: $(USAGE_FILES) $(MAIN_BINARIES)

build/bin/%: build/obj/main/%.o build/obj/main/prefix.a build/obj/main/bin_common.o
	@mkdir -p build/bin
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

bin/%_usage.h: bin/%.c docs/user_manual.md
	$(info Generating usage from manual...: $@)
	metasrc/show_usages_extract.py $< | pandoc -f markdown -t plain | metasrc/show_usages_txt_to_c.py > $@

# Libraries

lib: build/obj/main/prefix.a build/obj/main/prefix.so

build/obj/main/prefix.a: $(SRC_O_FILES)
	$(AR) -rcs $@ $^

build/obj/main/prefix.so: $(SRC_O_FILES)
	$(CC) $(LDFLAGS) -fvisibility=hidden -shared $^ -o $@
	
build/obj/main/%.o: %.c
	@mkdir -p build/obj/main
	$(CC) -o $@ -c $(CFLAGS) $<

# Coverage report

build/coverage_baseline.info: build/unittest fuzzers
	lcov -q -c -i --checksum --directory build/obj/test/ --directory build/obj/fuzzing_coverage/ \
		--output-file build/coverage_baseline.info

build/coverage_test.info: build/unittest
	lcov -q -z --directory build/obj/test/
	build/unittest
	lcov -q -c --checksum --directory build/obj/test/ --output-file build/coverage_test.info

build/coverage_fuzz.info: fuzzers
	lcov -q -z --directory build/obj/fuzzing_coverage/
	metasrc/extract_fuzzing_coverage.sh
	lcov -q -c --checksum --directory build/obj/fuzzing_coverage/ --output-file build/coverage_fuzz.info

build/coverage.info: build/coverage_baseline.info build/coverage_test.info build/coverage_fuzz.info
	lcov -q -a build/coverage_baseline.info -a build/coverage_test.info -a build/coverage_fuzz.info \
		 -o build/coverage.info

coverage_report: build/coverage.info
	genhtml -q build/coverage.info --output-directory build/coverage_report

# Test

test: build/unittest
	
build/unittest.report: build/unittest
	valgrind -q --track-fds=yes --partial-loads-ok=no --leak-check=yes build/unittest | tee build/unittest.report 

build/valgrind_fuzzing.report: fuzzers
	metasrc/valgrind_fuzzing_samples.sh | tee build/valgrind_fuzzing.report

build/unittest: $(TEST_O_FILES)
	$(CC) $(TEST_LDFLAGS) $^ -lcunit $(TEST_LDLIBS) -o $@

build/obj/test/%.o: %.c
	@mkdir -p build/obj/test
	$(CC) $(TEST_CFLAGS) -c $^ -o $@

# Fuzzers

fuzzers: $(FUZZER_BINARIES)

# Regular fuzzing harnesses
build/fuzzers/%: build/obj/fuzzing/%.o $(FUZZING_O_FILES)
	@mkdir -p build/fuzzers
	#AFL_QUIET=1 $(FUZZ_CC) $(FUZZ_LDFLAGS) $^ $(FUZZ_LDLIBS) -o $@
	#AFL_DONT_OPTIMIZE=1 AFL_QUIET=1 $(FUZZ_CC) $(FUZZ_LDFLAGS) $^ $(FUZZ_LDLIBS) -o $@
	AFL_QUIET=1 AFL_HARDEN=1 $(FUZZ_CC) $(FUZZ_LDFLAGS) $^ $(FUZZ_LDLIBS) -rtlib=compiler-rt -lgcc_s -o $@
	#AFL_QUIET=1 AFL_USE_MSAN=1 $(FUZZ_CC) $(FUZZ_LDFLAGS) $^ $(LDLIBS) -o $@

# This one links against libfiu
build/fuzzers/command_line_fiu_fuzzer: build/obj/fuzzing/command_line_fiu_fuzzer.o $(FUZZING_O_FILES)
	@mkdir -p build/fuzzers
	AFL_QUIET=1 AFL_HARDEN=1 $(FUZZ_CC) $(FUZZ_LDFLAGS) $^ $(FUZZ_LDLIBS) -l fiu -rtlib=compiler-rt \
		-lgcc_s -rdynamic -o $@

build/obj/fuzzing/%.o: %.c $(USAGE_FILES)
	@mkdir -p build/obj/fuzzing
	#AFL_QUIET=1 $(FUZZ_CC) -o $@ -c $(FUZZ_CFLAGS) -DNDEBUG $<
	#AFL_DONT_OPTIMIZE=1 AFL_QUIET=1 $(FUZZ_CC) -o $@ -c $(FUZZ_CFLAGS) -DNDEBUG $<
	AFL_QUIET=1 AFL_HARDEN=1 $(FUZZ_CC) -o $@ -c $(FUZZ_CFLAGS) $<
	#AFL_QUIET=1 AFL_USE_MSAN=1 $(FUZZ_CC) -o $@ -c $(FUZZ_CFLAGS) $<

# Fuzzers harnesses instrumented for code coverage (and not for fuzzing)
build/fuzzers/%_coverage: build/obj/fuzzing_coverage/%.o $(FUZZING_COV_O_FILES)
	@mkdir -p build/fuzzers
	$(CC) $(FUZZ_COV_LDFLAGS) $^ $(FUZZ_LDLIBS) -o $@

build/fuzzers/command_line_fiu_fuzzer_coverage: build/obj/fuzzing_coverage/command_line_fiu_fuzzer.o \
		$(FUZZING_COV_O_FILES)
	@mkdir -p build/fuzzers
	$(CC) $(FUZZ_COV_LDFLAGS) $^ $(FUZZ_LDLIBS) -l fiu -rdynamic -o $@

build/obj/fuzzing_coverage/%.o: %.c $(USAGE_FILES)
	@mkdir -p build/obj/fuzzing_coverage
	$(CC) $(FUZZ_COV_CFLAGS) -c $< -o $@


# Clean

clean:
	rm -fr build/bin build/coverage_report build/fuzzers build/obj build/pdf build/unittest build/doxygen SCF/ \
		build/reference_database.csv bin/*_usage.h ./crossvalidation/README_crossvalidation.pdf ./testdata/.generated \
		build/requirements_traceability_matrix.md ./testdata/{unittest_samples,prebuilt_codecs}
