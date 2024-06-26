// SPDX-FileCopyrightText: 2013 - 2024 NVIDIA CORPORATION. All Rights Reserved.
//
// SPDX-License-Identifier: BSD-3-Clause

#include "amgx_types/util.h"
#include "amgx_types/math.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////   Device-level generalized utils, should be included only in files compiled by nvcc  ////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace utils
{

#if 1
#define DEFAULT_MASK 0xffffffff
#else
// cannot use as default parameter to functions
#define DEFAULT_MASK activemask()
#endif

// ====================================================================================================================
// Atomics.
// ====================================================================================================================

static __device__ __forceinline__ void atomic_add( float *address, float value )
{
    atomicAdd( address, value );
}

static __device__ __forceinline__ void atomic_add( double *address, double value )
{
#if __CUDA_ARCH__ >= 600
    atomicAdd( address, value );
#else
    unsigned long long *address_as_ull = (unsigned long long *) address;
    unsigned long long old = __double_as_longlong( address[0] ), assumed;

    do
    {
        assumed = old;
        old = atomicCAS( address_as_ull, assumed, __double_as_longlong( value + __longlong_as_double( assumed ) ) );
    }
    while ( assumed != old );

#endif
}

static __device__ __forceinline__ void atomic_add( cuComplex *address, cuComplex value )
{
    atomicAdd((float *)(address), cuCrealf(value));
    atomicAdd((float *)((char *)(address) + sizeof(float)), cuCimagf(value));
}

static __device__ __forceinline__ void atomic_add( cuDoubleComplex *address, cuDoubleComplex value )
{
    atomic_add((double *)(address), cuCreal(value));
    atomic_add((double *)((char *)(address) + sizeof(double)), cuCimag(value));
}

static __device__ __forceinline__ int64_t atomic_CAS(int64_t* address, int64_t compare, int64_t val)
{
    return (int64_t)atomicCAS((unsigned long long *)address, (unsigned long long)compare, (unsigned long long)val);
}

static __device__ __forceinline__ int atomic_CAS(int* address, int compare, int val)
{
    return atomicCAS(address, compare, val);
}


// ====================================================================================================================
// Bit tools.
// ====================================================================================================================

static __device__ __forceinline__ int bfe( int src, int num_bits )
{
    unsigned mask;
    asm( "bfe.u32 %0, %1, 0, %2;" : "=r"(mask) : "r"(src), "r"(num_bits) );
    return mask;
}

static __device__ __forceinline__ int bfind( int src )
{
    int msb;
    asm( "bfind.u32 %0, %1;" : "=r"(msb) : "r"(src) );
    return msb;
}

static __device__ __forceinline__ int bfind( unsigned long long src )
{
    int msb;
    asm( "bfind.u64 %0, %1;" : "=r"(msb) : "l"(src) );
    return msb;
}

static __device__ __forceinline__ unsigned long long brev( unsigned long long src )
{
    return __brevll(src);
}

// ====================================================================================================================
// Warp tools.
// ====================================================================================================================

static __device__ __forceinline__ int lane_id()
{
    int id;
    asm( "mov.u32 %0, %%laneid;" : "=r"(id) );
    return id;
}

static __device__ __forceinline__ int lane_mask_lt()
{
    int mask;
    asm( "mov.u32 %0, %%lanemask_lt;" : "=r"(mask) );
    return mask;
}

static __device__ __forceinline__ int warp_id()
{
    return threadIdx.x >> 5;
}

// ====================================================================================================================
// Loads.
// ====================================================================================================================

enum Ld_mode { LD_AUTO = 0, LD_CA, LD_CG, LD_TEX, LD_NC };

template< Ld_mode Mode >
struct Ld {};

template<>
struct Ld<LD_AUTO>
{
    template< typename T >
    static __device__ __forceinline__ T load( const T *ptr ) { return *ptr; }
};

template<>
struct Ld<LD_CG>
{
    static __device__ __forceinline__ int load( const int *ptr )
    {
        return __ldcg(ptr);
    }

    static __device__ __forceinline__ float load( const float *ptr )
    {
        return __ldcg(ptr);
    }

    static __device__ __forceinline__ double load( const double *ptr )
    {
        return __ldcg(ptr);
    }

    static __device__ __forceinline__ cuComplex load( const cuComplex *ptr )
    {
        float2* vptr = (float2*) ptr;
        float2 ret = __ldcg(vptr);
        return make_cuComplex(ret.x, ret.y);
    }

    static __device__ __forceinline__ cuDoubleComplex load( const cuDoubleComplex *ptr )
    {
        double2* vptr = (double2*) ptr;
        double2 ret = __ldcg(vptr);
        return make_cuDoubleComplex(ret.x, ret.y);
    }

};

template<>
struct Ld<LD_CA>
{
    static __device__ __forceinline__ int load( const int *ptr )
    {
        return __ldca(ptr);
    }

    static __device__ __forceinline__ float load( const float *ptr )
    {
        return __ldca(ptr);
    }

    static __device__ __forceinline__ double load( const double *ptr )
    {
        return __ldca(ptr);
    }

    static __device__ __forceinline__ cuComplex load( const cuComplex *ptr )
    {
        float2* vptr = (float2*) ptr;
        float2 ret = __ldca(vptr);
        return make_cuComplex(ret.x, ret.y);
    }

    static __device__ __forceinline__ cuDoubleComplex load( const cuDoubleComplex *ptr )
    {
        double2* vptr = (double2*) ptr;
        double2 ret = __ldca(vptr);
        return make_cuDoubleComplex(ret.x, ret.y);
    }
};

template<>
struct Ld<LD_NC>
{
    template< typename T >
    static __device__ __forceinline__ T load( const T *ptr ) { return __ldg( ptr ); }
};


// ====================================================================================================================
// Vector loads.
// ====================================================================================================================

static __device__ __forceinline__ void load_vec2( float (&u)[2], const float *ptr )
{
    float2* vptr = (float2*) ptr;
    float2 ret = __ldcg(vptr);
    u[0] = ret.x;
    u[1] = ret.y;
}

static __device__ __forceinline__ void load_vec2( double (&u)[2], const double *ptr )
{
    double2* vptr = (double2*) ptr;
    double2 ret = __ldcg(vptr);
    u[0] = ret.x;
    u[1] = ret.y;
}

static __device__ __forceinline__ void load_vec4( float (&u)[4], const float *ptr )
{
    float4 * vptr = (float4 *) ptr;
    float4 ret = __ldcg(vptr);
    u[0] = ret.x;
    u[1] = ret.y;
    u[2] = ret.z;
    u[3] = ret.w;
}

static __device__ __forceinline__ void load_vec4( double (&u)[4], const double *ptr )
{
    double2 * vptr = (double2 *) ptr;
    double2 ret = __ldcg(vptr);
    u[0] = ret.x;
    u[1] = ret.y;
    vptr = (double2 *) (ptr+2);
    ret = __ldcg(vptr);
    u[2] = ret.x;
    u[3] = ret.y;
}

// ====================================================================================================================
// Warp vote functions
// ====================================================================================================================
static __device__ __forceinline__ unsigned int ballot(int p, unsigned int mask = DEFAULT_MASK)
{
#if CUDART_VERSION >= 9000
    return __ballot_sync(mask, p);
#else
    return __ballot(p);   
#endif
}

static __device__ __forceinline__ unsigned int any(int p, unsigned int mask = DEFAULT_MASK)
{
#if CUDART_VERSION >= 9000
    return __any_sync(mask, p);
#else
    return __any(p);   
#endif
}

static __device__ __forceinline__ unsigned int all(int p, unsigned int mask = DEFAULT_MASK)
{
#if CUDART_VERSION >= 9000
    return __all_sync(mask, p);
#else
    return __all(p);   
#endif
}

static __device__ __forceinline__ unsigned int activemask()
{
#if CUDART_VERSION >= 9000
    return __activemask();
#else
    return 0xffffffff;
#endif
}

static __device__ __forceinline__ void syncwarp(unsigned int mask = 0xffffffff)
{
#if CUDART_VERSION >= 9000
    return __syncwarp(mask);
#else
    return;
#endif
}

// ====================================================================================================================
// Shuffle.
// ====================================================================================================================
static __device__ __forceinline__ int shfl( int r, int lane, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    return __shfl_sync( mask, r, lane, bound );
#else
    return __shfl( r, lane, bound );
#endif
}


static __device__ __forceinline__ float shfl( float r, int lane, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    return __shfl_sync( mask, r, lane, bound );
#else
    return __shfl( r, lane, bound );
#endif
}

static __device__ __forceinline__ double shfl( double r, int lane, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    return __shfl_sync(mask, r, lane, bound );
#else
    int hi = __shfl( __double2hiint(r), lane, bound );
    int lo = __shfl( __double2loint(r), lane, bound );
    return __hiloint2double( hi, lo );
#endif
}

static __device__ __forceinline__ cuComplex shfl( cuComplex r, int lane, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    float re = __shfl_sync( mask, cuCrealf(r), lane, bound );
    float im = __shfl_sync( mask, cuCimagf(r), lane, bound );
    return make_cuComplex(re, im);
#else
    float re = __shfl( cuCrealf(r), lane, bound );
    float im = __shfl( cuCimagf(r), lane, bound );
    return make_cuComplex(re, im);
#endif
}

static __device__ __forceinline__ cuDoubleComplex shfl( cuDoubleComplex r, int lane, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
    double re = shfl( cuCreal(r), lane, mask, bound );
    double im = shfl( cuCimag(r), lane, mask, bound );
    return make_cuDoubleComplex( re, im );
}

static __device__ __forceinline__ int shfl_xor( int r, int lane_mask, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    return __shfl_xor_sync( mask, r, lane_mask, bound );
#else
    return __shfl_xor( r, lane_mask, bound );
#endif
}


static __device__ __forceinline__ float shfl_xor( float r, int lane_mask, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    return __shfl_xor_sync( mask, r, lane_mask, bound );
#else
    return __shfl_xor( r, lane_mask, bound );
#endif
}

static __device__ __forceinline__ double shfl_xor( double r, int lane_mask, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    return __shfl_xor_sync( mask, r, lane_mask, bound );
#else
    int hi = __shfl_xor( __double2hiint(r), lane_mask, bound );
    int lo = __shfl_xor( __double2loint(r), lane_mask, bound );
    return __hiloint2double( hi, lo );
#endif
}

static __device__ __forceinline__ cuComplex shfl_xor( cuComplex r, int lane_mask, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    float re = __shfl_xor_sync( mask, cuCrealf(r), lane_mask, bound );
    float im = __shfl_xor_sync( mask, cuCimagf(r), lane_mask, bound );
    return make_cuComplex(re, im);
#else
    float re = __shfl_xor( cuCrealf(r), lane_mask, bound );
    float im = __shfl_xor( cuCimagf(r), lane_mask, bound );
    return make_cuComplex(re, im);
#endif
}

static __device__ __forceinline__ cuDoubleComplex shfl_xor( cuDoubleComplex r, int lane_mask, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
    double re = shfl_xor( cuCreal(r), lane_mask, mask, bound );
    double im = shfl_xor( cuCimag(r), lane_mask, mask, bound );
    return make_cuDoubleComplex( re, im );
}

static __device__ __forceinline__ int shfl_down( int r, int offset, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    return __shfl_down_sync( mask, r, offset, bound );
#else
    return __shfl_down( r, offset, bound );
#endif
}

static __device__ __forceinline__ float shfl_down( float r, int offset, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    return __shfl_down_sync( mask, r, offset, bound );
#else
    return __shfl_down( r, offset, bound );
#endif
}

static __device__ __forceinline__ double shfl_down( double r, int offset, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    return __shfl_down_sync( mask, r, offset, bound );
#else
    int hi = __shfl_down( __double2hiint(r), offset, bound );
    int lo = __shfl_down( __double2loint(r), offset, bound );
    return __hiloint2double( hi, lo );
#endif
}

static __device__ __forceinline__ cuComplex shfl_down( cuComplex r, int lane, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    float re = __shfl_down_sync( mask, cuCrealf(r), lane, bound );
    float im = __shfl_down_sync( mask, cuCimagf(r), lane, bound );
    return make_cuComplex(re, im);
#else
    float re = __shfl_down( cuCrealf(r), lane, bound );
    float im = __shfl_down( cuCimagf(r), lane, bound );
    return make_cuComplex(re, im);
#endif
}

static __device__ __forceinline__ cuDoubleComplex shfl_down( cuDoubleComplex r, int lane, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
    double re = shfl_down( cuCreal(r), lane, bound );
    double im = shfl_down( cuCimag(r), lane, bound );
    return make_cuDoubleComplex( re, im );
}


static __device__ __forceinline__ int shfl_up( int r, int offset, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    return __shfl_up_sync( mask, r, offset, bound );
#else
    return __shfl_up( r, offset, bound );
#endif
}

static __device__ __forceinline__ float shfl_up( float r, int offset, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    return __shfl_up_sync( mask, r, offset, bound );
#else
    return __shfl_up( r, offset, bound );
#endif
}

static __device__ __forceinline__ double shfl_up( double r, int offset, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    return __shfl_up_sync( mask, r, offset, bound );
#else
    int hi = __shfl_up( __double2hiint(r), offset, bound );
    int lo = __shfl_up( __double2loint(r), offset, bound );
    return __hiloint2double( hi, lo );
#endif
}

static __device__ __forceinline__ cuComplex shfl_up( cuComplex r, int lane, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
#if CUDART_VERSION >= 9000
    float re = __shfl_up_sync( mask, cuCrealf(r), lane, bound );
    float im = __shfl_up_sync( mask, cuCimagf(r), lane, bound );
    return make_cuComplex(re, im);
#else
    float re = __shfl_up( cuCrealf(r), lane, bound );
    float im = __shfl_up( cuCimagf(r), lane, bound );
    return make_cuComplex(re, im);
#endif
}

static __device__ __forceinline__ cuDoubleComplex shfl_up( cuDoubleComplex r, int lane, int bound = warpSize, unsigned int mask = DEFAULT_MASK )
{
    double re = shfl_up( cuCreal(r), lane, bound );
    double im = shfl_up( cuCimag(r), lane, bound );
    return make_cuDoubleComplex( re, im );
}

// ====================================================================================================================
// Warp-level reductions.
// ====================================================================================================================

struct Add
{
    template< typename Value_type >
    static __device__ __forceinline__ Value_type eval( Value_type x, Value_type y ) { return x + y; }
};

template< int NUM_THREADS_PER_ITEM, int WarpSize >
struct Warp_reduce_pow2
{
    template< typename Operator, typename Value_type >
    static __device__ __inline__ Value_type execute( Value_type x )
    {
#pragma unroll

        for ( int mask = WarpSize / 2 ; mask >= NUM_THREADS_PER_ITEM ; mask >>= 1 )
        {
            x = Operator::eval( x, shfl_xor(x, mask) );
        }

        return x;
    }
};

template< int NUM_THREADS_PER_ITEM, int WarpSize >
struct Warp_reduce_linear
{
    template< typename Operator, typename Value_type >
    static __device__ __inline__ Value_type execute( Value_type x )
    {
        const int NUM_STEPS = WarpSize / NUM_THREADS_PER_ITEM;
        int my_lane_id = utils::lane_id();
#pragma unroll

        for ( int i = 1 ; i < NUM_STEPS ; ++i )
        {
            Value_type y = shfl_down( x, i * NUM_THREADS_PER_ITEM );

            if ( my_lane_id < NUM_THREADS_PER_ITEM )
            {
                x = Operator::eval( x, y );
            }
        }

        return x;
    }
};

// ====================================================================================================================

template< int NUM_THREADS_PER_ITEM, int WarpSize = 32 >
struct Warp_reduce : public Warp_reduce_pow2<NUM_THREADS_PER_ITEM, WarpSize> {};

template< int WarpSize >
struct Warp_reduce< 3, WarpSize> : public Warp_reduce_linear< 3, WarpSize> {};

template< int WarpSize >
struct Warp_reduce< 4, WarpSize> : public Warp_reduce_linear< 4, WarpSize> {};

template< int WarpSize >
struct Warp_reduce< 5, WarpSize> : public Warp_reduce_linear< 5, WarpSize> {};

template< int WarpSize >
struct Warp_reduce< 6, WarpSize> : public Warp_reduce_linear< 6, WarpSize> {};

template< int WarpSize >
struct Warp_reduce< 7, WarpSize> : public Warp_reduce_linear< 7, WarpSize> {};

template< int WarpSize >
struct Warp_reduce< 9, WarpSize> : public Warp_reduce_linear< 9, WarpSize> {};

template< int WarpSize >
struct Warp_reduce<10, WarpSize> : public Warp_reduce_linear<10, WarpSize> {};

template< int WarpSize >
struct Warp_reduce<11, WarpSize> : public Warp_reduce_linear<11, WarpSize> {};

template< int WarpSize >
struct Warp_reduce<12, WarpSize> : public Warp_reduce_linear<12, WarpSize> {};

template< int WarpSize >
struct Warp_reduce<13, WarpSize> : public Warp_reduce_linear<13, WarpSize> {};

template< int WarpSize >
struct Warp_reduce<14, WarpSize> : public Warp_reduce_linear<14, WarpSize> {};

template< int WarpSize >
struct Warp_reduce<15, WarpSize> : public Warp_reduce_linear<15, WarpSize> {};

// ====================================================================================================================

template< int NUM_THREADS_PER_ITEM, typename Operator, typename Value_type >
static __device__ __forceinline__ Value_type warp_reduce( Value_type x )
{
    return Warp_reduce<NUM_THREADS_PER_ITEM>::template execute<Operator>( x );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace utils

