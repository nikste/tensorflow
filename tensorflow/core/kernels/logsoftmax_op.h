/* Copyright 2015 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_KERNELS_LOGSOFTMAX_OP_H_
#define TENSORFLOW_KERNELS_LOGSOFTMAX_OP_H_
// Functor definition for LogSoftmaxOp, must be compilable by nvcc.

#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#include "tensorflow/core/framework/tensor_types.h"

namespace tensorflow {
namespace functor {

// Functor used by LogSoftmaxOp to do the computations.
template <typename Device, typename T>
struct LogSoftmaxFunctor {
  // Computes LogSoftmax activation.
  //
  // logits: dim: batch_size, num_classes.
  // logsoftmax: dims: batch_size, num_classes.
  void operator()(const Device& d, typename TTypes<T>::ConstMatrix logits,
                  typename TTypes<T>::Matrix logsoftmax);
};

// Eigen code implementing LogSoftmaxFunctor::operator().
// This code works for both CPU and GPU and is used by the functor
// specializations for both device types.
template <typename Device, typename T>
struct LogSoftmaxEigenImpl {
  static void Compute(const Device& d, typename TTypes<T>::ConstMatrix logits,
                      typename TTypes<T>::Matrix logsoftmax) {
    const int kBatchDim = 0;
    const int kClassDim = 1;

    const int batch_size = logits.dimension(kBatchDim);
    const int num_classes = logits.dimension(kClassDim);

// These arrays are used to reduce along the class dimension, and broadcast
// the resulting value to all classes.
#if !defined(EIGEN_HAS_INDEX_LIST)
    Eigen::DSizes<int, 1> along_class(kClassDim);
    Eigen::DSizes<int, 2> batch_by_one(batch_size, 1);
    Eigen::DSizes<int, 2> one_by_class(1, num_classes);
#else
    Eigen::IndexList<Eigen::type2index<kClassDim> > along_class;
    Eigen::IndexList<Eigen::type2index<1> > depth_dim;
    Eigen::IndexList<int, Eigen::type2index<1> > batch_by_one;
    batch_by_one.set(0, batch_size);
    Eigen::IndexList<Eigen::type2index<1>, int> one_by_class;
    one_by_class.set(1, num_classes);
#endif
    // NOTE(touts): If you modify this implementation please run
    // the ImageNetLogSoftmaxFwd benchmark in core_ops_test.cc.
    //
    // logsoftmax = exp(logits - max(logits along classes));
    logsoftmax.device(d) = (logits -
                         logits.maximum(along_class)
                             .eval()
                             .reshape(batch_by_one)
                             .broadcast(one_by_class))
                            .exp();
    // logsoftmax = logsoftmax / sum(logsoftmax along classes);
    logsoftmax.device(d) = (logsoftmax /
                         logsoftmax.sum(along_class)
                             .eval()
                             .reshape(batch_by_one)
                             .broadcast(one_by_class));
  }
};

}  // namespace functor
}  // namespace tensorflow

#endif  // TENSORFLOW_KERNELS_LOGSOFTMAX_OP_H_
