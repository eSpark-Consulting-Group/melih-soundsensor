#include <gtest/gtest.h>

#include <Array.h>

#ifdef COUNTOF
#error COUNTOF redefinition!
#else
#define COUNTOF( array ) sizeof( array ) / sizeof( array[0] )
#endif

constexpr uint32_t ArrayLength = 8;
constexpr uint32_t ResizeToLength = 3;
constexpr uint32_t ValueToPushBack = 7;

using MyArray = common::Array<int, ArrayLength>;

static MyArray a;

TEST( Array, capacity ) {
    EXPECT_EQ( ArrayLength, a.capacity() );
}

TEST( Array, size ) {
    EXPECT_EQ( 0, a.size() );
}

TEST( Array, resize ) {
    a.resize( ResizeToLength );
    EXPECT_EQ( ResizeToLength, a.size() );
}

static const MyArray::value_type buf[] = { 1, 2, 3, 2 };

TEST( Array, copyFrom ) {
    auto result = a.copyFrom( buf, COUNTOF( buf ) );
    EXPECT_TRUE( result );

    result = a.copyFrom( buf, COUNTOF( buf ) );
    EXPECT_FALSE( result );

    EXPECT_EQ( ResizeToLength + COUNTOF( buf ), a.size() );

    for( int i = 0; i < ResizeToLength; ++i ) {
        EXPECT_EQ( 0, a[i] );
    }

    for( int i = 0; i < COUNTOF( buf ); ++i ) {
        EXPECT_EQ( buf[i], a[ResizeToLength + i] );
    }
}

TEST( Array, pushBack ) {
    auto result = a.pushBack( ValueToPushBack );
    EXPECT_TRUE( result );

    result = a.pushBack( ValueToPushBack );
    EXPECT_FALSE( result );

    EXPECT_EQ( 1 + ResizeToLength + COUNTOF( buf ), a.size() );
    EXPECT_EQ( ValueToPushBack, a[a.size() - 1] );
}

TEST( Array, eraseAndShift ) {
    a.eraseAndShift( ResizeToLength - 1 );

    EXPECT_EQ( ResizeToLength + COUNTOF( buf ), a.size() );

    for( int i = 0; i < ResizeToLength - 1; ++i ) {
        EXPECT_EQ( 0, a[i] );
    }

    for( int i = 0; i < COUNTOF( buf ); ++i ) {
        EXPECT_EQ( buf[i], a[ResizeToLength - 1 + i] );
    }

    EXPECT_EQ( ValueToPushBack, a[a.size() - 1] );
}

TEST( Array, eraseAndSwap ) {
    a.eraseAndSwap( ResizeToLength - 2 );

    EXPECT_EQ( ResizeToLength - 1 + COUNTOF( buf ), a.size() );

    for( int i = 0; i < ResizeToLength - 2; ++i ) {
        EXPECT_EQ( 0, a[i] );
    }

    EXPECT_EQ( ValueToPushBack, a[ResizeToLength - 2] );

    for( int i = 0; i < COUNTOF( buf ); ++i ) {
        EXPECT_EQ( buf[i], a[ResizeToLength - 1 + i] );
    }
}

TEST( Array, find ) {
    static const unsigned int index = 1;
    static const auto value = buf[index];
    auto i = a.find( []( int& a ) -> bool { return a == value; } );

    EXPECT_NE( i, a.end() );
    EXPECT_EQ( *i, value );
    EXPECT_EQ( i - a.begin(), ResizeToLength - 1 + index );

    auto constTest = []( const decltype( a )& a ) -> void {
        auto i = a.find( []( const int& a ) -> bool { return a == value; } );

        EXPECT_NE( i, a.end() );
        EXPECT_EQ( *i, value );
        EXPECT_EQ( i - a.begin(), ResizeToLength - 1 + index );
    };

    constTest( a );
}

TEST( Array, findNext ) {
    static const unsigned int index = 1;
    static const auto value = buf[index];
    auto i = a.find( []( int& a ) -> bool { return a == value; } );

    EXPECT_NE( i, a.end() );
    EXPECT_EQ( *i, value );
    EXPECT_EQ( i - a.begin(), ResizeToLength - 1 + index );

    i = a.findNext( i + 1, []( int& a ) -> bool { return a == value; } );
    EXPECT_NE( i, a.end() );
    EXPECT_EQ( *i, value );
    EXPECT_EQ( i - a.begin(), a.size() - 1 );

    auto constTest = []( const decltype( a )& a ) -> void {
        auto i = a.find( []( const int& a ) -> bool { return a == value; } );

        EXPECT_NE( i, a.end() );
        EXPECT_EQ( *i, value );
        EXPECT_EQ( i - a.begin(), ResizeToLength - 1 + index );

        i = a.findNext( i + 1, []( const int& a ) -> bool { return a == value; } );
        EXPECT_NE( i, a.end() );
        EXPECT_EQ( *i, value );
        EXPECT_EQ( i - a.begin(), a.size() - 1 );
    };

    constTest( a );
}

int main( int argc, char** argv ) {
    ::testing::InitGoogleTest( &argc, argv );

    return RUN_ALL_TESTS();
}
