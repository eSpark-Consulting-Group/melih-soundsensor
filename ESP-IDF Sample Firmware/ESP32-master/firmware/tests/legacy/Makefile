MKDIR := "mkdir"
GPP   := "g++"
AR    := "ar"
RM    := "rm"
GPP_OPTS := -o0 -g3

#
# Folder to put the test binaries to
#

GTEST_OUTPUT_DIR := $(PROJECT_DIR)/build/tests

#
# Every sub-folder is considered as a separate test suite
# The name of a sub-folder is expected to be name of the corresponding binary
#

#
# Note: the 'dummy' is a reference sample so let's exclude it
#
 
EXCLUDE_TESTS := dummy

SUBDIRS := $(wildcard */.)
SUBDIRS := $(SUBDIRS:/.=)
SUBDIRS := $(filter-out $(EXCLUDE_TESTS),$(SUBDIRS))

TEST_BINARIES := $(addprefix $(GTEST_OUTPUT_DIR)/, $(SUBDIRS))

#
# Create the output folder
#

$(GTEST_OUTPUT_DIR):
	$(MKDIR) -p $(GTEST_OUTPUT_DIR)

#
# Assemble the Google Test framework as a library
#

$(GTEST_OUTPUT_DIR)/gtest-all.o: $(GTEST_OUTPUT_DIR)
	$(GPP) $(GPP_OPTS) -isystem ${GTEST_DIR}/googletest/include -I${GTEST_DIR}/googletest -pthread -c ${GTEST_DIR}/googletest/src/gtest-all.cc -o $@

$(GTEST_OUTPUT_DIR)/libgtest.a: $(GTEST_OUTPUT_DIR)/gtest-all.o
	$(AR) -rv $@ $<

#
# Assemble all the binaries of the tests
#

$(SUBDIRS): $(GTEST_OUTPUT_DIR)/libgtest.a
	$(MAKE) -C $@  PROJECT_DIR=$(PROJECT_DIR) GTEST_OUTPUT_DIR=$(GTEST_OUTPUT_DIR) GTEST_DIR=$(GTEST_DIR) all

#
# Launch all the binaries of the tests
#

$(TEST_BINARIES): $(SUBDIRS) $(GTEST_OUTPUT_DIR)/libgtest.a
	$@

#
# Remove the Google Test framework library all the binaries of the tests
#

clean:
	$(RM) -f $(GTEST_OUTPUT_DIR)/gtest-all.o
	$(RM) -f $(GTEST_OUTPUT_DIR)/libgtest.a
	for xdir in  $(SUBDIRS) ; do \
		$(MAKE) -C $${xdir}  PROJECT_DIR=$(PROJECT_DIR) GTEST_OUTPUT_DIR=$(GTEST_OUTPUT_DIR) GTEST_DIR=$(GTEST_DIR) clean ; \
	done

#
# 'gtest' is the entry target to perform build and running of the tests
#

gtest: $(TEST_BINARIES) $(SUBDIRS) $(GTEST_OUTPUT_DIR)/libgtest.a

.PHONY: all clean $(SUBDIRS) $(TEST_BINARIES)
