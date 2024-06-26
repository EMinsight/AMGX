// SPDX-FileCopyrightText: 2011 - 2024 NVIDIA CORPORATION. All Rights Reserved.
//
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#define BSIZE 4
#define BSIZE_SQ 16

#include <matrix.h>
#include "amgx_types/math.h"

namespace amgx
{

// solves Ax = b via gaussian elimination
template<class TConfig>
struct GaussianElimination
{
    static void gaussianElimination(const Matrix<TConfig> &A, Vector<TConfig> &x, const Vector<TConfig> &b);
};

// host specialization
template <AMGX_VecPrecision t_vecPrec, AMGX_MatPrecision t_matPrec, AMGX_IndPrecision t_indPrec>
struct GaussianElimination<TemplateConfig<AMGX_host, t_vecPrec, t_matPrec, t_indPrec> >
{
        typedef TemplateConfig<AMGX_host, t_vecPrec, t_matPrec, t_indPrec> TConfig_h;
        typedef Matrix<TConfig_h> Matrix_h;
        typedef Vector<TConfig_h> Vector_h;
        static void gaussianElimination(const Matrix_h &A, Vector_h &x, const Vector_h &b);
    private:
        static void gaussianElimination_4x4_host(const Matrix_h &A, Vector_h &x, const Vector_h &b);
};

// device specialization
template <AMGX_VecPrecision t_vecPrec, AMGX_MatPrecision t_matPrec, AMGX_IndPrecision t_indPrec>
struct GaussianElimination<TemplateConfig<AMGX_device, t_vecPrec, t_matPrec, t_indPrec> >
{
        typedef TemplateConfig<AMGX_device, t_vecPrec, t_matPrec, t_indPrec> TConfig_d;
        typedef Matrix<TConfig_d> Matrix_d;
        typedef Vector<TConfig_d> Vector_d;
        static void gaussianElimination(const Matrix_d &A, Vector_d &x, const Vector_d &b);
    private:
        static void gaussianElimination_4x4_device(const Matrix_d &A, Vector_d &x, const Vector_d &b);
};

// solves Ax = b via gaussian elimination
template<typename IndexType, typename ValueTypeA, typename ValueTypeB>
void gaussianEliminationRowMajor(ValueTypeA **e, ValueTypeB *x, ValueTypeB *b, const IndexType bsize);

template <typename ValueType>
__host__ __device__ inline void gaussianElimination4by4(ValueType (&E)[BSIZE][BSIZE], ValueType(&x)[BSIZE], ValueType(&b)[BSIZE] )
{
    ValueType pivot, ratio, tmp;
    // j=0
    pivot = E[0][0];
    // k=1
    ratio = E[1][0] / pivot;
    b[1] = b[1] - b[0] * ratio;
    E[1][1] = E[1][1] - E[0][1] * ratio;
    E[1][2] = E[1][2] - E[0][2] * ratio;
    E[1][3] = E[1][3] - E[0][3] * ratio;
    // k=2
    ratio = E[2][0] / pivot;
    b[2] = b[2] - b[0] * ratio;
    E[2][1] = E[2][1] - E[0][1] * ratio;
    E[2][2] = E[2][2] - E[0][2] * ratio;
    E[2][3] = E[2][3] - E[0][3] * ratio;
    //k=3
    ratio = E[3][0] / pivot;
    b[3] = b[3] - b[0] * ratio;
    E[3][1] = E[3][1] - E[0][1] * ratio;
    E[3][2] = E[3][1] - E[0][2] * ratio;
    E[3][3] = E[3][1] - E[0][3] * ratio;
    // j=1
    pivot = E[1][1];
    // k=2
    ratio = E[2][1] / pivot;
    b[2] = b[2] - b[1] * ratio;
    E[2][2] = E[2][2] - E[1][2] * ratio;
    E[2][3] = E[2][3] - E[1][3] * ratio;
    // k=3
    ratio = E[3][1] / pivot;
    b[3] = b[3] - b[1] * ratio;
    E[3][2] = E[3][2] - E[1][2] * ratio;
    E[3][3] = E[3][3] - E[1][3] * ratio;
    // j=2
    pivot = E[2][2];
    // k=3
    ratio = E[3][2] / pivot;
    b[3] = b[3] - b[2] * ratio;
    E[3][3] = E[3][3] - E[2][3] * ratio;
    // back substitution
    // j=3
    x[3] = b[3] / E[3][3];
    // j=2
    tmp = E[2][3] * x[3];
    x[2] = (b[2] - tmp) / E[2][2];
    // j=1
    tmp = E[1][2] * x[2] + E[1][3] * x[3];
    x[1] = (b[1] - tmp) / E[1][1];
    // j=0
    tmp = E[0][1] * x[1] + E[0][2] * x[2] + E[0][3] * x[3];
    x[0] = (b[0] - tmp) / E[0][0];
}

} // namespace amgx
