#ifndef TENSOR_TENSOR_IMPL_H
#define TENSOR_TENSOR_IMPL_H

#include "shape.h"
#include "storage.h"
#include "array.h"
#include "allocator.h"

#include <initializer_list>

namespace st {
    class TensorImpl {
    public:
        class const_iterator {};
        class iterator {};
        // constructor
        TensorImpl(const Storage& Storage, const Shape& Shape, const IndexArray& stride);
        TensorImpl(const Storage& Storage, const Shape& Shape);
        explicit TensorImpl(const Shape& Shape);
        TensorImpl(const data_t* data, const Shape& Shape);
        TensorImpl(Storage&& Storage, Shape&& Shape, IndexArray&& stride);
        TensorImpl(const TensorImpl& other) = delete;
        TensorImpl(TensorImpl&& other) = default;

        // inline function
        [[nodiscard]] index_t n_dim() const { return _shape.n_dim(); }
        [[nodiscard]] index_t d_size() const { return  _shape.d_size(); }
        [[nodiscard]] index_t size(index_t idx) const { return _shape[idx]; }
        [[nodiscard]] const Shape& size() const { return _shape; }
        [[nodiscard]] index_t offset() const { return _storage.offset(); }
        [[nodiscard]] const IndexArray& stride() const { return _stride; }

        // methods
        bool is_contiguous();
        TensorImpl self() const; // only copy value

        data_t& operator[](std::initializer_list<index_t> dims); // use initializer list to access/modify the data.
        data_t operator[](std::initializer_list<index_t> dims) const;
        [[nodiscard]] data_t item() const;
        [[nodiscard]] data_t item(const int idx) const;

        [[nodiscard]] Alloc::NonTrivalUniquePtr<TensorImpl> slice(index_t idx, index_t dim = 0) const;
        [[nodiscard]] Alloc::NonTrivalUniquePtr<TensorImpl> slice(index_t start_idx, index_t end_idx, index_t dim) const;
        [[nodiscard]] Alloc::NonTrivalUniquePtr<TensorImpl> transpose(index_t dim1, index_t dim2) const;
        [[nodiscard]] Alloc::NonTrivalUniquePtr<TensorImpl> view(const Shape& Shape) const;
        [[nodiscard]] Alloc::NonTrivalUniquePtr<TensorImpl> permute(std::initializer_list<index_t> dims) const;

        // friend function
        friend std::ostream& operator<<(std::ostream& out, const TensorImpl& tensor);
        friend TensorImpl operator+(const TensorImpl& tensor1, const TensorImpl& tensor2);
        friend TensorImpl operator-(const TensorImpl& tensor1, const TensorImpl& tensor2);
        friend TensorImpl operator*(int v, const TensorImpl& tensor);
        friend TensorImpl operator*(const TensorImpl& tensor, int v);
        friend TensorImpl operator/(const TensorImpl& tensor, int v);

    protected:
        Storage _storage;
        Shape _shape;
        IndexArray _stride;
    };

} // SimpleTensor

#endif //TENSOR_TENSOR_IMPL_H
