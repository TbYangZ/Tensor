#include <memory>
#include "tensor.h"
#include "exp.h"
#include "exception.h"

namespace st
{
	//constructors
	Tensor::Tensor(const Storage& storage, const Shape& shape, const IndexArray& stride) :
		Exp<TensorImpl>(Alloc::unique_construct<TensorImpl>(storage, shape, stride)) {}
	Tensor::Tensor(const Storage& storage, const Shape& shape) :
		Exp<TensorImpl>(Alloc::unique_construct<TensorImpl>(storage, shape)) {}
	Tensor::Tensor(const Shape& shape) :
        Exp<TensorImpl>(Alloc::unique_construct<TensorImpl>(shape)) {}
	Tensor::Tensor(const data_t* data, const Shape& shape) :
        Exp<TensorImpl>(Alloc::unique_construct<TensorImpl>(data, shape)) {}
	Tensor::Tensor(Storage&& storage, Shape&& shape, IndexArray&& stride) :
        Exp<TensorImpl>(Alloc::unique_construct<TensorImpl>(std::move(storage), std::move(shape), std::move(stride))) {}
	Tensor::Tensor(Alloc::NonTrivalUniquePtr<TensorImpl>&& ptr) : Exp<TensorImpl>(std::move(ptr)) {}

	//operations
	bool Tensor::is_contiguous() { return impl_ptr->is_contiguous(); }
	data_t Tensor::item() const { return impl_ptr->item(); }
	data_t Tensor::item(const int idx) const { return impl_ptr->item(idx); }
	data_t &Tensor::operator[](std::initializer_list<index_t> dims) { return impl_ptr->operator[](dims); }
	data_t Tensor::operator[](std::initializer_list<index_t> dims) const { return impl_ptr->operator[](dims); }

	Tensor Tensor::slice(index_t idx, index_t dim) const
	{
		return Tensor(impl_ptr->slice(idx, dim));
	}
	Tensor Tensor::slice(index_t start, index_t end, index_t dim) const
	{
		return Tensor(impl_ptr->slice(start, end, dim));
	}
	Tensor Tensor::view(const Shape& shape) const
	{
		return Tensor(impl_ptr->view(shape));
	}
	Tensor Tensor::transpose(index_t dim1, index_t dim2) const
	{
		return Tensor(impl_ptr->transpose(dim1, dim2));
	}
	Tensor Tensor::permute(std::initializer_list<index_t> dims) const
	{
		return Tensor(impl_ptr->permute(dims));
	}
    Tensor Tensor::sum(int idx) const {
        return Tensor(impl_ptr->sum(idx));
    }
	std::ostream& operator<<(std::ostream& out, const Tensor& tensor)
	{
		out << *tensor.impl_ptr;
		return out;
	}


	//iterator
	Tensor::iterator::iterator(Tensor* tensor, std::vector<index_t> idx)
	{
        CHECK_EQUAL(idx.size(), tensor->n_dim(), "Index size not match.");
		_tensor = tensor;
        for (index_t i = 0; i < idx.size(); ++i) {
            CHECK_IN_RANGE(i, 0, tensor->size()[i], "Index out of range.");
            _idx.push_back(idx[i]);
        }
	}

	Tensor::iterator& Tensor::iterator::operator++()
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_TRUE(*this != _tensor->end(), "Iterator out of range.");
		int cnt = 0;
		for (int i = (int)_idx.size()-1; i >= 0; --i)
		{
			if (_idx[i]+1 < _tensor->size()[i])
			{
				++_idx[i];
				break;
			}
			else
			{
				_idx[i] = 0;
				++cnt;
			}
		}
		if (cnt == _idx.size())
		{
			for (int i = 0; i < _idx.size(); ++i)
			{
				_idx[i] = _tensor->size()[i];
			}
		}
		return *this;
	}

	Tensor::iterator Tensor::iterator::operator++(int)
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_TRUE(*this != _tensor->end(), "Iterator out of range.");
		iterator tmp = *this;
		++*this;
		return tmp;
	}

	Tensor::iterator& Tensor::iterator::operator--()
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_TRUE(*this != _tensor->begin(), "Iterator out of range.");
		if (*this == _tensor->end())
		{
			for (int i = 0; i < _idx.size(); ++i)
			{
				_idx[i] = _tensor->size()[i]-1;
			}
			return *this;
		}
		for (int i = 0; i < _idx.size(); --i)
		{
			if (_idx[i] > 0)
			{
				--_idx[i];
				break;
			}
			else
			{
				_idx[i] = _tensor->size()[i]-1;
			}
		}
		return *this;
	}

	Tensor::iterator Tensor::iterator::operator--(int)
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_TRUE(*this != _tensor->begin(), "Iterator out of range.");
		iterator tmp = *this;
		--*this;
		return tmp;
	}

	bool Tensor::iterator::operator==(const iterator& other) const
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_EQUAL(other._idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_EQUAL(_tensor, other._tensor, "Tensor not match.");
		return _idx == other._idx;
	}

	bool Tensor::iterator::operator!=(const iterator& other) const
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_EQUAL(other._idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_EQUAL(_tensor, other._tensor, "Tensor not match.");
		return !(*this == other);
	}

	Tensor::iterator::reference Tensor::iterator::operator*() const
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_TRUE(*this != _tensor->end(), "Iterator out of range.");
		index_t idx = 0;
		for (int i = 0; i < _idx.size(); ++i)
		{
			idx += _idx[i] * _tensor->impl_ptr->stride()[i];
		}
		return _tensor->impl_ptr->item(idx);
	}

	Tensor::iterator::pointer Tensor::iterator::operator->() const
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_TRUE(*this != _tensor->end(), "Iterator out of range.");
		return &**this;
	}

	//const_iterator
	Tensor::const_iterator::const_iterator(const Tensor* tensor, std::vector<index_t> idx)
	{
        CHECK_EQUAL(idx.size(), tensor->n_dim(), "Index size not match.");
		_tensor = tensor;
		for (index_t i = 0; i < idx.size(); ++i) {
            CHECK_IN_RANGE(i, 0, tensor->size()[i], "Index out of range.");
            _idx.push_back(idx[i]);
        }
	}
	Tensor::const_iterator& Tensor::const_iterator::operator++()
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_TRUE(*this != _tensor->end(), "Iterator out of range.");
        int cnt = 0;
		for (int i = (int)_idx.size()-1; i >= 0; --i)
		{
			if (_idx[i]+1 < _tensor->size()[i])
			{
				++_idx[i];
				break;
			}
			else
			{
				_idx[i] = 0;
				++cnt;
			}
		}
		if (cnt == _idx.size())
		{
			for (int i = 0; i < _idx.size(); ++i)
			{
				_idx[i] = _tensor->size()[i];
			}
		}
		return *this;
	}

	Tensor::const_iterator Tensor::const_iterator::operator++(int)
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_TRUE(*this != _tensor->end(), "Iterator out of range.");
        const_iterator tmp = *this;
		++*this;
		return tmp;
	}

	Tensor::const_iterator& Tensor::const_iterator::operator--()
	{
		CHECK_IN_RANGE(_idx.size(), 0, _tensor->n_dim(), "Index out of range.");
        CHECK_TRUE(*this != _tensor->begin(), "Iterator out of range.");
        if (*this == _tensor->end())
		{
			for (int i = 0; i < _idx.size(); ++i)
			{
				_idx[i] = _tensor->size()[i]-1;
			}
			return *this;
		}
		for (int i = 0; i < _idx.size(); --i)
		{
			if (_idx[i] > 0)
			{
				--_idx[i];
				break;
			}
			else
			{
				_idx[i] = _tensor->size()[i]-1;
			}
		}
		return *this;
	}

	Tensor::const_iterator Tensor::const_iterator::operator--(int)
	{
		CHECK_IN_RANGE(_idx.size(), 0, _tensor->n_dim(), "Index out of range.");
        CHECK_TRUE(*this != _tensor->begin(), "Iterator out of range.");
        const_iterator tmp = *this;
		--*this;
		return tmp;
	}

	bool Tensor::const_iterator::operator==(const const_iterator& other) const
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_EQUAL(other._idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_EQUAL(_tensor, other._tensor, "Tensor not match.");
        return _idx == other._idx && _tensor == other._tensor;
	}

	bool Tensor::const_iterator::operator!=(const const_iterator& other) const
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_EQUAL(other._idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_EQUAL(_tensor, other._tensor, "Tensor not match.");
        return !(*this == other);
	}

	Tensor::const_iterator::const_reference Tensor::const_iterator::operator*() const
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_TRUE(*this != _tensor->end(), "Iterator out of range.");
        index_t idx = 0;
		for (int i = 0; i < _idx.size(); ++i)
		{
			idx += _idx[i] * _tensor->impl_ptr->stride()[i];
		}
		return _tensor->impl_ptr->item(idx);
	}

	Tensor::const_iterator::const_pointer Tensor::const_iterator::operator->() const
	{
        CHECK_EQUAL(_idx.size(), _tensor->n_dim(), "Index size not match.");
        CHECK_TRUE(*this != _tensor->end(), "Iterator out of range.");
        return &**this;
	}
	//iterator_methods

	Tensor::iterator Tensor::begin()
	{
		return iterator(this, std::vector<index_t>(impl_ptr->n_dim()));
	}

	Tensor::iterator Tensor::end()
	{
		std::vector<index_t> idx;
		for (int i = 0; i < impl_ptr->n_dim(); ++i)
		{
			idx.push_back(impl_ptr->size()[i]);
		}
		return iterator(this, idx);
	}

	Tensor::const_iterator Tensor::begin() const
	{
		return const_iterator(this, std::vector<index_t>(impl_ptr->n_dim()));
	}

	Tensor::const_iterator Tensor::end() const
	{
		std::vector<index_t> idx;
		for (int i = 0; i < impl_ptr->n_dim(); ++i)
		{
			idx.push_back(impl_ptr->size()[i]);
		}
		return const_iterator(this, idx);
	}

	data_t Tensor::eval(IndexArray idx) const
	{
        return impl_ptr->eval(idx);
	}

    data_t Tensor::sum() const {
        return impl_ptr->sum();
    }

    Tensor Tensor::rand(const st::Shape &shape) {
        return Tensor(Alloc::unique_construct<TensorImpl>(TensorMaker::rand(shape)));
    }
    Tensor Tensor::ones(const st::Shape &shape) {
        return Tensor(Alloc::unique_construct<TensorImpl>(TensorMaker::ones(shape)));
    }
    Tensor Tensor::zeros(const st::Shape &shape) {
        return Tensor(Alloc::unique_construct<TensorImpl>(TensorMaker::zeros(shape)));
    }
	Tensor Tensor::zeros_like(const st::Tensor& tensor)
	{
		return Tensor(Alloc::unique_construct<TensorImpl>(TensorMaker::zeros_like(*(tensor.impl_ptr))));
	}
	Tensor Tensor::ones_like(const st::Tensor& tensor)
	{
		return Tensor(Alloc::unique_construct<TensorImpl>(TensorMaker::ones_like(*(tensor.impl_ptr))));
	}
	Tensor Tensor::rand_like(const st::Tensor& tensor)
	{
		return Tensor(Alloc::unique_construct<TensorImpl>(TensorMaker::rand_like(*(tensor.impl_ptr))));
	}
    Tensor Tensor::randn(const Shape &shape) {
        return Tensor(Alloc::unique_construct<TensorImpl>(TensorMaker::randn(shape)));
    }
    Tensor Tensor::randn_like(const Tensor &tensor) {
        return Tensor(Alloc::unique_construct<TensorImpl>(TensorMaker::randn_like(*(tensor.impl_ptr))));
    }

} // SimpleTensor