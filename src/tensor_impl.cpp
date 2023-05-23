#include "tensor_impl.h"
#include "exception.h"
#include <memory>
#include <cmath>
#include <iomanip>
#include <random>
#include <ctime>

#define debug printf("%d %s\n", __LINE__, __FUNCTION__)

namespace st {
    // constructor
    TensorImpl::TensorImpl(const Storage& storage, const Shape& shape, const IndexArray& stride) :
        _storage(storage), _shape(shape), _stride(stride) {}
    TensorImpl::TensorImpl(const Storage& storage, const Shape& shape) :
        _storage(storage), _shape(shape), _stride(shape.n_dim()){
        for (int i = 0; i < shape.n_dim(); ++i) {
            if (i == shape.n_dim()-1) _stride[i] = 1;
            else _stride[i] = shape.sub_size(i+1);
            if (shape[i] == 1) _stride[i] = 0;
        }
    }
    TensorImpl::TensorImpl(const Shape& shape) :
        _storage(shape.d_size()), _shape(shape), _stride(shape.n_dim()) {
        for (int i = 0; i < shape.d_size(); ++i)
            _storage[i] = 0;
        for (int i = 0; i < shape.n_dim(); ++i) {
            if (i == shape.n_dim()-1) _stride[i] = 1;
            else _stride[i] = shape.sub_size(i+1);
            if (shape[i] == 1) _stride[i] = 0;
        }
    }
    TensorImpl::TensorImpl(const data_t* data, const Shape& shape) :
        _storage(shape.d_size()), _shape(shape), _stride(shape.n_dim()) {
        for (int i = 0; i < shape.d_size(); ++i)
            _storage[i] = data[i];
        for (int i = 0; i < shape.n_dim(); ++i) {
            if (i == shape.n_dim()-1) _stride[i] = 1;
            else _stride[i] = shape.sub_size(i+1);
            if (shape[i] == 1) _stride[i] = 0;
        }
    }
    TensorImpl::TensorImpl(Storage&& storage, Shape&& shape, IndexArray&& stride) :
        _storage(std::move(storage)), _shape(std::move(shape)), _stride(std::move(stride)) {}

    // method
    bool TensorImpl::is_contiguous() const
	{
        for (int i = 0; i < n_dim()-1; ++i) {
            if (_shape[i] == 1) continue;
            if (_stride[i] != _shape.sub_size(i+1)) return false;
        }
        if (_shape[n_dim()-1] != 1 && _stride[n_dim()-1] != 1) return false;
        return true;
    }

    data_t& TensorImpl::operator[](std::initializer_list<index_t> dims) {
		CHECK_EQUAL(n_dim(), dims.size(),
				"Invalid %zuD indices for %dD tensor", dims.size(), n_dim());
        index_t index = 0, dim = 0;
        for (auto v : dims) {
            CHECK_IN_RANGE(v, 0, size(dim),
                           "Index out of range (expected to be in range of [0, %d), but got %d)",
                           size(dim), v);
            index += v*_stride[dim];
            ++dim;
        }
        return _storage[index];
    }
    data_t TensorImpl::operator[](std::initializer_list<index_t> dims) const {
		CHECK_EQUAL(n_dim(), dims.size(),
			"Invalid %zuD indices for %dD tensor", dims.size(), n_dim());
        index_t index = 0, dim = 0;
        for (auto v : dims) {
            std::cout << v << " ";
            CHECK_IN_RANGE(v, 0, size(dim),
                           "Index out of range (expected to be in range of [0, %d), but got %d)",
                           size(dim), v);
            index += v*_stride[dim];
            ++dim;
        }
        std::cout << std::endl;
        return _storage[index];
    }

    data_t TensorImpl::item() const {
		CHECK_TRUE(n_dim() == 1 && size(0) == 1,
			"Only one element tensors can be converted to scalars");
        return _storage[0];
    }

   data_t TensorImpl::item(index_t idx) const {
        return _storage[idx];
	}

	data_t& TensorImpl::item(index_t idx)
	{
		return _storage[idx];
	}
	data_t TensorImpl::eval(IndexArray idx) const {
        int index = 0;
        if (idx.size() >= _shape.n_dim()) {
            for (int i = idx.size() - n_dim(); i < idx.size(); ++i)
                index += idx[i]*_stride[i-(idx.size()-n_dim())];
        } else {
            for (int i = 0; i < idx.size(); ++i)
                index += idx[i]*_stride[i+(n_dim()-idx.size())];
        }
        return item(index);
    }

    Alloc::NonTrivalUniquePtr<TensorImpl>
    TensorImpl::slice(index_t idx, index_t dim) const {
		CHECK_IN_RANGE(dim, 0, n_dim(),
			"Dimension out of range (expected to be in range of [0, %d), but got %d)",
			n_dim(), dim);
		CHECK_IN_RANGE(idx, 0, size(dim),
			"Index %d is out of bound for dimension %d with size %d",
			idx, dim, size(dim));
        Alloc::NonTrivalUniquePtr<TensorImpl> ptr;
        ptr = Alloc::unique_construct<TensorImpl>(
                Storage(_storage, offset() + _stride[dim] * idx),
                _shape, _stride);
        ptr->_shape[dim] = 1;
        ptr->_stride[dim] = 0;
        return ptr;
    }

    Alloc::NonTrivalUniquePtr<TensorImpl>
    TensorImpl::slice(index_t start_idx, index_t end_idx, index_t dim) const {
		CHECK_IN_RANGE(dim, 0, n_dim(),
			"Dimension out of range (expected to be in range of [0, %d), but got %d)",
			n_dim(), dim);
		CHECK_IN_RANGE(start_idx, 0, size(dim),
			"Index %d is out of bound for dimension %d with size %d",
			start_idx, dim, size(dim));
		CHECK_IN_RANGE(end_idx, 0, size(dim)+1,
			"Range end %d is out of bound for dimension %d with size %d",
			end_idx, dim, size(dim));
        CHECK_TRUE(start_idx < end_idx,
                   "slice() expects the start index must be smaller than the end index");
        Alloc::NonTrivalUniquePtr<TensorImpl> ptr;
        ptr = Alloc::unique_construct<TensorImpl>(
                Storage(_storage, offset() + start_idx * _stride[dim]),
                _shape, _stride);
        ptr->_shape[dim] = end_idx-start_idx;
        return ptr;
    }

    Alloc::NonTrivalUniquePtr<TensorImpl>
    TensorImpl::transpose(index_t dim1, index_t dim2) const {
		CHECK_IN_RANGE(dim1, 0, n_dim(),
			"Dimension out of range (expected to be in range of [0, %d), but got %d)",
			n_dim(), dim1);
		CHECK_IN_RANGE(dim2, 0, n_dim(),
			"Dimension out of range (expected to be in range of [0, %d), but got %d)",
			n_dim(), dim2);
        Alloc::NonTrivalUniquePtr<TensorImpl> ptr;
        ptr = Alloc::unique_construct<TensorImpl>(_storage, _shape, _stride);
        std::swap(ptr->_shape[dim1], ptr->_shape[dim2]);
        std::swap(ptr->_stride[dim1], ptr->_stride[dim2]);
        return ptr;
    }

    Alloc::NonTrivalUniquePtr<TensorImpl>
    TensorImpl::view(const Shape &shape) const {
        CHECK_TRUE( is_contiguous(),
            "view() is only supported to contiguous tensor");
        CHECK_EQUAL(shape.d_size(), shape.d_size(),
            "Shape of size %d is invalid for input tensor with size %d",
            shape.d_size(), shape.d_size());
        Alloc::NonTrivalUniquePtr<TensorImpl> ptr;
        ptr = Alloc::unique_construct<TensorImpl>(_storage, shape);
        for (int i = 0; i < shape.n_dim(); ++i) {
            if (i+1 < shape.n_dim()) ptr->_stride[i] = shape.sub_size(i+1);
            else ptr->_stride[i] = 1;
			if (shape[i] == 1) ptr->_stride[i] = 0;
        }
        return ptr;
    }

    Alloc::NonTrivalUniquePtr<TensorImpl>
    TensorImpl::permute(std::initializer_list<index_t> dims) const {
		CHECK_EQUAL(dims.size(), n_dim(),
			"Dimension not match (expected dims of %d, but got %zu)",
			n_dim(), dims.size());
        Alloc::NonTrivalUniquePtr<TensorImpl> ptr;
        ptr = Alloc::unique_construct<TensorImpl>(_storage, _shape);
        int idx = 0;
        for (auto n_permute : dims) {
            ptr->_shape[idx] = _shape[n_permute];
            ptr->_stride[idx] = _stride[n_permute];
            ++idx;
        }
        return ptr;
    }

    Alloc::NonTrivalUniquePtr<TensorImpl>
    TensorImpl::sum(int idx) const {
        CHECK_IN_RANGE(idx, 0, n_dim(),
            "Dimension out of range (expected to be in range of [0, %d), but got %d)",
            n_dim(), idx);
        Alloc::NonTrivalUniquePtr<TensorImpl> ptr;
        ptr = Alloc::unique_construct<TensorImpl>(Shape(_shape, idx));
        std::vector<index_t> idxs(_shape.n_dim(), 0);
        std::vector<index_t> new_idxs(_shape.n_dim()-1, 0);
        index_t cnt = 0;
        while (cnt < d_size()) {
            data_t res = 0;
            for (int i = 0; i < _shape[idx]; ++i) {
                idxs[idx] = i;
                res += eval(idxs);
            }
            new_idxs.clear();
            for (int i = 0; i < n_dim(); ++i) {
                if (i == idx) continue;
                new_idxs.push_back(idxs[i]);
            }
            index_t index = 0;
            for (index_t i = 0; i < new_idxs.size(); ++i) {
                index += new_idxs[i] * ptr->_stride[i];
            }
            ptr->item(index) = res;
            for (int i = 0; i < n_dim(); ++i) {
                if (i == idx) continue;
                if (idxs[i] + 1 >= _shape[i]) idxs[i] = 0;
                else {
                    ++idxs[i];
                    break;
                }
            }
            cnt += _shape[idx];
        }
        return ptr;
    }

    // friend function
    std::ostream& operator<<(std::ostream& out, const TensorImpl& tensor) {
        int max_width = 0;
        for (int i = 0; i < tensor.d_size(); ++i) {
            int value = (int)std::abs(tensor.item(i));
            int dig = value = (int)(std::log10(value))+1;
            if (tensor.item(i) < 0) ++dig;
            max_width = std::max(max_width, dig);
        }
        int cnt = 0, idx = 0, end_flag = tensor.n_dim();
        std::vector<int> dim_cnt(tensor.n_dim());
        while (cnt < tensor.d_size()) {
            for (int i = 0; i < tensor.n_dim()-end_flag; ++i)
                out << " ";
            for (int i = 0; i < end_flag; ++i)
                out << "[";
            out << std::setw(max_width+4+1) << std::right << std::setprecision(4) << std::fixed;
            out << tensor.item(idx);
            end_flag = 0;
            for (int i = (int)tensor.n_dim()-1; i >= 0; --i) {
                if (dim_cnt[i]+1 < tensor.size()[i]) {
                    idx += tensor.stride()[i];
                    ++dim_cnt[i];
                    break;
                } else {
                    idx -= ((int)tensor.size()[i]-1)*tensor.stride()[i];
                    dim_cnt[i] = 0;
                    ++end_flag;
                }
            }
            if (end_flag == 0) out << ", ";
            else {
                for (int i = 0; i < end_flag; ++i) {
                    out << "]";
                }
                out << std::endl;

            }
            ++cnt;
        }
        return out;
    }

    data_t TensorImpl::sum() const {
        data_t res = 0;
        std::vector<index_t> idx(n_dim(), 0);
        for (int i = 0; i < d_size(); ++i) {
            int cnt = 0;
            res += eval(idx);
            for (int j = 0; j < n_dim(); ++j) {
                if (idx[j]+1 < size()[j]) {
                    ++idx[j];
                    break;
                } else {
                    idx[j] = 0;
                    ++cnt;
                }
            }
            if (cnt == n_dim()) break;
        }
        return res;
    }

    // TensorMaker
    TensorImpl TensorMaker::ones(const Shape &shape) {
        TensorImpl tensor(shape);
        for (int i = 0; i < tensor.d_size(); ++i)
            tensor.item(i) = 1;
        return tensor;
    }

    TensorImpl TensorMaker::ones_like(const TensorImpl &tensor) {
        return ones(tensor.size());
    }

    TensorImpl TensorMaker::zeros(const Shape &shape) {
        TensorImpl tensor(shape);
        for (int i = 0; i < tensor.d_size(); ++i)
            tensor.item(i) = 0;
        return tensor;
    }

    TensorImpl TensorMaker::zeros_like(const TensorImpl &tensor) {
        return zeros(tensor.size());
    }

    TensorImpl TensorMaker::rand(const Shape &shape) {
        std::random_device rd;
        std::default_random_engine gen(rd());
        std::uniform_real_distribution<data_t> dis(0, 1);
        TensorImpl tensor(shape);
        for (int i = 0; i < tensor.d_size(); ++i)
            tensor.item(i) = dis(gen);
        return tensor;
    }

    TensorImpl TensorMaker::rand_like(const TensorImpl &tensor) {
        return rand(tensor.size());
    }

    TensorImpl TensorMaker::randn(const Shape &shape) {
        std::random_device rd;
        std::default_random_engine gen(rd());
        std::normal_distribution<data_t> dis(0, 1);
        TensorImpl tensor(shape);
        for (int i = 0; i < tensor.d_size(); ++i)
            tensor.item(i) = dis(gen);
        return tensor;
    }

    TensorImpl TensorMaker::randn_like(const TensorImpl &tensor) {
        return randn(tensor.size());
    }
} // st