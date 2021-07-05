#ifndef MAT_TASK_TYPE_H
#define MAT_TASK_TYPE_H

#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <stdlib.h>
#include <unistd.h>
#include <chrono>
#include <queue>
#include <fstream>
#include <atomic>
#include <random>
#include <array>
#include <vector>

#include <cereal/cereal.hpp> // for defer
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>

namespace bip=boost::interprocess;
const int SIZE = 150;

// ================================================================================
// Util-functions
// ================================================================================


// ================================================================================
// Struct Definition
// ================================================================================

/* Struct of a matrix-tuple type using std::array */
typedef struct MatTask_Type {
    int tid;
    double A[SIZE*SIZE];
    double B[SIZE*SIZE];

    // constructor 1
    MatTask_Type(): tid(0), A(), B() {}

    // constructor 2, init matrices with identical values
    MatTask_Type(int id, int val):
            tid(id){}

    // constructor 3, init matrices with random values
    // TODO

    // serialization for using rpc lib
#ifdef HCL_ENABLE_RPCLIB
    MSGPACK_DEFINE(A,B);
#endif

} MatTask_Type;

// serialization like thallium does
#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
    template<typename A>
    void serialize(A &ar, MatTask_Type &a) {
        ar & a.tid;
        ar & a.A;
        ar & a.B;
    }
#endif



typedef struct MatResult_Type {
    int tid;
    double A[SIZE*SIZE];

    // constructor 1
    MatResult_Type(): tid(0), A() {}

    // constructor 2, init matrices with identical values
    MatResult_Type(int id, int val):
            tid(id){ }

    // constructor 3, init matrices with random values
    // TODO
    
    

    // serialization for using rpc lib
#ifdef HCL_ENABLE_RPCLIB
    MSGPACK_DEFINE(A);
#endif

} MatResult_Type;

// serialization like thallium does
#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
    template<typename A>
    void serialize(A &ar, MatResult_Type &a) {
        ar & a.tid;
        ar & a.A;
    }
#endif



#endif //MAT_TASK_TYPE_H
