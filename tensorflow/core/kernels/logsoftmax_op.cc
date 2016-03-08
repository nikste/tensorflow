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

// See docs in ../ops/nn_ops.cc.

#define EIGEN_USE_THREADS

#include "tensorflow/core/kernels/logsoftmax_op.h"
#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_shape.h"

namespace tensorflow {

    typedef Eigen::ThreadPoolDevice CPUDevice;
    typedef Eigen::GpuDevice GPUDevice;

    template <typename Device, typename T>
    class LogSoftmaxOp : public OpKernel {
    public:
        explicit LogSoftmaxOp(OpKernelConstruction* context) : OpKernel(context) {}

        void Compute(OpKernelContext* context) override {
            const Tensor& logits_in = context->input(0);
            OP_REQUIRES(context, TensorShapeUtils::IsMatrix(logits_in.shape()),
                        errors::InvalidArgument("logits must be 2-dimensional"));
            Tensor* logsoftmax_out = nullptr;
            OP_REQUIRES_OK(
                    context, context->allocate_output(0, logits_in.shape(), &logsoftmax_out));
            functor::LogSoftmaxFunctor<Device, T> functor;
            functor(context->eigen_device<Device>(), logits_in.matrix<T>(),
                    logsoftmax_out->matrix<T>());
        }
    };

// Partial specialization for a CPUDevice, that uses the Eigen implementation
// from LogSoftmaxEigenImpl.
    namespace functor {
        template <typename T>
        struct LogSoftmaxFunctor<CPUDevice, T> {
            void operator()(const CPUDevice& d, typename TTypes<T>::ConstMatrix logits,
                            typename TTypes<T>::Matrix logsoftmax) {
                LogSoftmaxEigenImpl<CPUDevice, T>::Compute(d, logits, logsoftmax);
            }
        };
    }  // namespace functor

    REGISTER_KERNEL_BUILDER(Name("LogSoftmax")
    .Device(DEVICE_CPU)
    .TypeConstraint<float>("T"),
    LogSoftmaxOp<CPUDevice, float>);
    REGISTER_KERNEL_BUILDER(Name("LogSoftmax")
    .Device(DEVICE_CPU)
    .TypeConstraint<double>("T"),
    LogSoftmaxOp<CPUDevice, double>);

#if GOOGLE_CUDA
    REGISTER_KERNEL_BUILDER(Name("LogSoftmax")
                            .Device(DEVICE_GPU)
                            .TypeConstraint<float>("T"),
                        LogSoftmaxOp<GPUDevice, float>);
#endif  // GOOGLE_CUDA

}  // namespace tensorflow
