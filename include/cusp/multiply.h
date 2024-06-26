// SPDX-FileCopyrightText: 2008 - 2024 NVIDIA CORPORATION. All Rights Reserved.
//
// SPDX-License-Identifier: BSD-3-Clause

/*! \file multiply.h
 *  \brief Matrix multiplication 
 */

#pragma once

#include <cusp/detail/config.h>

namespace cusp
{

/*! \addtogroup algorithms Algorithms
 *  \ingroup algorithms
 *  \{
 */

/*! \p multiply : Implements matrix-matrix and matrix-vector multiplication
 *
 * \p multiply can be used with dense matrices, sparse matrices, and user-defined
 * \p linear_operator objects.
 *
 * \param A input matrix
 * \param B input matrix or vector
 * \param C output matrix or vector
 *
 * \tparam LinearOperator matrix
 * \tparam MatrixOrVector1 matrix or vector
 * \tparam MatrixOrVector2 matrix or vector
 *
 *  The following code snippet demonstrates how to use \p multiply to
 *  compute a matrix-vector product.
 *
 *  \code
 *  #include <cusp/multiply.h>
 *  #include <cusp/array2d.h>
 *  #include <cusp/print.h>
 *  
 *  int main(void)
 *  {
 *      // initialize matrix
 *      cusp::array2d<float, cusp::host_memory> A(2,2);
 *      A(0,0) = 10;  A(0,1) = 20;
 *      A(1,0) = 40;  A(1,1) = 50;
 *  
 *      // initialize input vector
 *      cusp::array1d<float, cusp::host_memory> x(2);
 *      x[0] = 1;
 *      x[1] = 2;
 *  
 *      // allocate output vector
 *      cusp::array1d<float, cusp::host_memory> y(2);
 *  
 *      // compute y = A * x
 *      cusp::multiply(A, x, y);
 *  
 *      // print y
 *      cusp::print(y);
 *  
 *      return 0;
 *  }
 *  \endcode
 */
template <typename LinearOperator,
          typename MatrixOrVector1,
          typename MatrixOrVector2>
void multiply(LinearOperator&  A,
              MatrixOrVector1& B,
              MatrixOrVector2& C);
/*! \}
 */

} // end namespace cusp

#include <cusp/detail/multiply.inl>

