#include "gtest/gtest.h"

double dummy( double d ) {
    return d + d;
}

TEST( sample, dummy ) {
    EXPECT_EQ( 20, dummy( 10 ) );
}

int main( int argc, char** argv ) {
    ::testing::InitGoogleTest( &argc, argv );

    return RUN_ALL_TESTS();
}
