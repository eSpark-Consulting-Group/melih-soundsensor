GPP   := "g++"
RM    := "rm"
GPP_OPTS := -o0 -g3

all:
	$(GPP) $(GPP_OPTS) -isystem ${GTEST_DIR}/googletest/include -pthread DummyTest.cpp $(GTEST_OUTPUT_DIR)/libgtest.a -o $(GTEST_OUTPUT_DIR)/dummy

clean:
	$(RM) -f $(GTEST_OUTPUT_DIR)/dummy
