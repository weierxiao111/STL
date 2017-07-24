#pragma once
#include "simple_alloc.hpp"
template <class T,class Alloc=std::alloc>
class Vector {
public:
	typedef T  value_type;
	typedef value_type* pointer;
	typedef value_type* iterator;
	typedef value_type& reference;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
protected:
	typedef Simple_alloc<value_type, Alloc> data_allocator;
	iterator start;
	iterator finish;
	iterator end_of_storage;

	void insert_aux(iterator position, const T& x);
	void deallocate()
	{
		if (start)
			data_allocator::deallocate(start, end_of_storage - start);
	}

	void fill_initialize(size_type n, const T& value)
	{
		start = allocate_and_fill(n, value);
		finish = start + n;
		end_of_storage = finish;
	}

protected:
	iterator allocate_and_fill(size_type n, const T& x)
	{
		iterator restual = data_allocator::allocate(n);
		uninitialized_fill_n(restual, n, x);
		return restual;
	}
};
