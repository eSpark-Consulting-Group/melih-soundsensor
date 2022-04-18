
/*

Usage:

 let's assume the following struct is to be packet/unpacked

 struct A {
     uint8_t a;
     uint16_t b;
 };

 The following 3 steps shall be done in order to use the pack/unpack helpers

 (3-1) declare list of members as a macro,
     note:
         [1] all the fields are assumed to be of integral type
         [2] it is a good practice to assign proper initial values to each field

 #define STRUCT_A_MEMBERS \
     uint8_t a = 0; \
     uint16_t b = 0;

 (3-2) declare the both of the packed and unpacked structs ; these couple of steps (1,2) is very important
     to keep the packed and unpacked structs consistent

 DECLARE_STRUCTS_PAIR( A, PackedA, STRUCT_A_MEMBERS );

 as a result of a macro substitution, the following structs will be declared:

 struct A;
 typedef struct {
     uint8_t a;
     uint16_t b;
 } __attribute__( ( packed ) ) PackedABase;

 struct PackedA : public PackedABase {
    PackedA& operator=( const A& other );
 };
 struct A {
     uint8_t a;
     uint16_t b;
     A& operator=( const PackedA& other );
 };

 (3-3) somewhere in the implementation file, the packing/unpacking code can be generated via the following macroes
 A& A::operator=( const PackedA& src ) {
     UNPACK_STRUCTURE_FIELD( &src, this, a );
     UNPACK_STRUCTURE_FIELD( &src, this, b );

     return *this;
 }

 PackedA& PackedA::operator=( const A& src ) {
     PACK_STRUCTURE_FIELD( &src, this, a );
     PACK_STRUCTURE_FIELD( &src, this, b );

     return *this;
 }

Example:
  NodeInfo.h
  NodeInfo.cpp

 */

#pragma once

#include <stdint.h>

#include <type_traits>
#include <cstring>

namespace common {

struct StructToImage {
    template <class Field>
    static uint32_t unpackField( const Field* src, Field& dest ) {
        memcpy( &dest, src, sizeof( dest ) );

        return sizeof( dest );
    }

    template <class Field>
    static uint32_t packField( const Field& src, Field* dest ) {
        memcpy( dest, &src, sizeof( *dest ) );

        return sizeof( *dest );
    }
};

};  // namespace common

#if defined( PACK_STRUCTURE_FIELD ) || defined( UNPACK_STRUCTURE_FIELD ) || defined( DECLARE_STRUCTS_PAIR )
#error Redefinition of PACK_STRUCTURE_FIELD, UNPACK_STRUCTURE_FIELD, DECLARE_STRUCTS_PAIR macroes
#else
#define PACK_STRUCTURE_FIELD( originPtr, destinationPtr, field )                                               \
    do {                                                                                                       \
        common::StructToImage::packField<decltype( ( destinationPtr )->field )>( ( originPtr )->field,         \
                                                                                 &( destinationPtr )->field ); \
    } while( 0 )

#define UNPACK_STRUCTURE_FIELD( originPtr, destinationPtr, field )                                              \
    do {                                                                                                        \
        common::StructToImage::unpackField<decltype( ( destinationPtr )->field )>( &( originPtr )->field,       \
                                                                                   ( destinationPtr )->field ); \
    } while( 0 )

#define DECLARE_STRUCTS_PAIR( Unpacked, Packed, MemberList ) \
    struct Unpacked;                                         \
    typedef struct {                                         \
        MemberList;                                          \
    } __attribute__( ( packed ) ) Packed##Base;              \
    struct Packed : public Packed##Base {                    \
        Packed& operator=( const Unpacked& other );          \
    };                                                       \
    struct Unpacked {                                        \
        MemberList;                                          \
        Unpacked& operator=( const Packed& other );          \
    };

#endif
