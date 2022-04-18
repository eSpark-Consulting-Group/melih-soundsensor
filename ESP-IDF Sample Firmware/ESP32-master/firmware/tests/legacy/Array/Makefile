GPP   := "g++"
RM    := "rm"
GPP_OPTS := -o0 -g3

INCLUDE_DIRS := $(PROJECT_DIR)/components/common/include
INCLUDES := $(addprefix -I, $(INCLUDE_DIRS))

all:
	$(GPP) $(GPP_OPTS) -isystem ${GTEST_DIR}/googletest/include $(INCLUDES) -pthread ArrayTest.cpp $(GTEST_OUTPUT_DIR)/libgtest.a -o $(GTEST_OUTPUT_DIR)/Array

clean:
	$(RM) -f $(GTEST_OUTPUT_DIR)/Array
