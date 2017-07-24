#pragma once
#include <iostream>
using namespace std;

//当用户申请内存大于128时使用一级空间配置器
template <int inst>
class malloc_Alloc_template {   
public:
	static void *allocate(size_t n)
	{
		void *result = malloc(n);
		if (n == 0)
			result = oom_malloc(n); //不满足要求进行处理
		return result;
	}
	static void dellocate(void *p, size_t)
	{
		free(p);
	}

	static void *reallocate(void *p, size_t old_size, size_t new_size)
	{
		void *result = realloc(p, new_size);
		if (0 == result)
			result = oom_realloc(p, new_size); 
		return result;
	}


	static void(*set_malloc_handler(void(*f)()))()//用户可以自己设定
		                                         //没有内存时重新分配的函数
	{
		void(*old)() = alloc_oom_handler;
		alloc_oom_handler = f;
		return old;
	}

private:
	static  void(* alloc_oom_handler)();
private:
	static void *oom_malloc(size_t n)
	{
		void *result;
		void(*f)(); 

		for (;;)
		{
			f = alloc_oom_handler;
			if (NULL == f)
			{
				cerr << "out of merroy" << endl;
				exit(1);
			}
			(*f)();
			result = malloc(n);
			if (result)
				return result;
		}
	}

	static void oom_realloc(void *p, size_t new_size)
	{
		void *result;
		void(*f)();

		for (;;)
		{
			f = alloc_oom_handler;
			if (NULL == f)
			{
				cerr << "out of merroy" << endl;
				exit(1);
			}
			*f();
			result = rellocc(p, old_size);
			if (result)
				return result;
		}
	}
};

template <int inst>
void(*malloc_Alloc_template<inst>::alloc_oom_handler)() = NULL; //静态成员初始化

typedef malloc_Alloc_template<0> malloc_Alloc;

//二级空间配置器

enum { ALIGN = 8 }; //小区块上界
enum { MAX_BYTES = 128 };  //区块上限
enum { FREELIST = MAX_BYTES / ALIGN }; //小区块个数

template <int inst>
class default_Alloc_template {
public:
	static void *allocate(size_t n);
	static void *reallocate(void *p, size_t old_size, size_t new_size);
	static void deallocate(void *p, size_t n);
private:
	//用于提升申请内存大小到小区块上界的倍数
	static size_t ROUND_UP(size_t bytes)
	{
		return ((bytes + ALIGN - 1) & ~(ALIGN - 1));
	}
	//下标
	static size_t FREELIST_INDEX(size_t bytes)
	{
		return ((bytes + ALIGN - 1) / ALIGN - 1);
	}

	static void *refill(size_t n);
	static char* chunk_alloc(size_t size, int &nobjs);

private:
	union obj{
		union obj* free_list_link;
		char client_data[1];
	};
private:
	static obj*  volatile free_list[FREELIST];
	static char* start_free; //内存池起始位置
	static char* end_free;  //内存池结束位置  
	static size_t heap_size;
};

template <int inst>
char *default_Alloc_template<inst>::start_free = 0;

template <int inst>
char *default_Alloc_template<inst>::end_free = 0;

template <int inst>
size_t default_Alloc_template<inst>::heap_size = 0;

template <int inst>
typename default_Alloc_template<inst>::obj* volatile
default_Alloc_template<inst>::free_list[FREELIST] = {0};

template <int inst>
void* default_Alloc_template<inst>::allocate(size_t n)
{
	obj* volatile *my_free_list;
	obj* result;
	// 大于128调用一级空间配置器
	if (n > (size_t)MAX_BYTES)
	{
		return (malloc_Alloc::allocate(n));
	}
	//判断应该使用my_free_list应该使用16个中的哪一个
	my_free_list = free_list + FREELIST_INDEX(n);
	result = *my_free_list;

	//没有找到，说明没有所需要的free_list
	if (0 == result)
	{
		void *r = refill(ROUND_UP(n));
		return r;
	}
	//有所需要的free_list,free_list指向下一个 
	*my_free_list = result->free_list_link;
	return result;
}

template <int inst>
void default_Alloc_template<inst>::deallocate(void *p, size_t n)
{
	obj *q = (obj *)p;
	obj * volatile *my_free_list;
	//大于128调用1级空间配置器的函数
	if (n > (size_t)FREELIST)
	{
		malloc_Alloc::dellocate(p, n);
		return;
	}
	my_free_list = free_list + FREELIST_INDEX(n);
	q->free_list_link = *my_free_list;
	*my_free_list = q;
}

//freelist没有可用的区块时，调用refill填充新空间
template <int inst>
void* default_Alloc_template<inst>::refill(size_t n)
{
	//默认取得20个区块
	int nobjs = 20;
	//从内存池取得区块给free_list使用
	char *chunk = chunk_alloc(n, nobjs);
	obj* volatile *my_free_list;
	obj* result;
	obj* cur_obj;
	obj* next_obj;
	int i;
	//只取得了一个，直接交给调用者
	if (1 == nobjs)
		return chunk;
	my_free_list = free_list + FREELIST_INDEX(n);
	result = (obj*)chunk;
	*my_free_list = next_obj = (obj*)(chunk + n);
	//将剩余的区块交给free_list，各节点连接起来
	for (i = 1;; ++i)
	{
		cur_obj = next_obj;
		next_obj = (obj*)((char*)next_obj + n);
		if (nobjs - 1 == i)
		{
			cur_obj->free_list_link = 0;
			break;
		}
		else
		{
			cur_obj->free_list_link = next_obj;
		}
	}
	return result;
}
//从内存池取得空间交给free_list使用
template <int inst>
char *default_Alloc_template<inst>::chunk_alloc(size_t size, int &nobjs)
{
	char *result;
	size_t total_bytes = size*nobjs; //需要的空间
	size_t bytes_left = end_free - start_free; //内存池剩余的空间
	
	//剩余空间足够
	if (bytes_left >= total_bytes)
	{
		result = start_free;
		start_free += total_bytes;
		return (result);
	}
	//剩余空间不够，但是能满足一个小区块
	else if (bytes_left >= size)
	{
		//能够得到的区块个数
		nobjs = bytes_left/ size;
		total_bytes = nobjs*size;
		result = start_free;
		start_free += total_bytes;
		return result;
	}
	//剩余内存无法提供1个大小的区块
	else
	{
		size_t bytes_toget = 2 * total_bytes + ROUND_UP(heap_size >> 4);
		if (bytes_left > 0)
		{
			obj* volatile *my_free_list = free_list
				+ FREELIST_INDEX(bytes_left);
			((obj *)start_free)->free_list_link = *my_free_list;
			*my_free_list = (obj*)start_free;
		}
		//配置heap空间，用来补充内存池
		start_free = (char*)malloc(bytes_toget);
		if (0 == start_free)
		{
			//heap空间不足，malloc失败
			int i;
			obj* volatile *my_free_list;
			obj* p;
			//从更大的区块尝试获取
			for (i = size; i <= MAX_BYTES; i += ALIGN)
			{
				my_free_list = free_list + FREELIST_INDEX(i);
				p = *my_free_list;
				if (0 != p)
				{
					*my_free_list = p->free_list_link;
					start_free = (char*)p;
					end_free = start_free + i;
					return chunk_alloc(size, nobjs);
				}
			}
			end_free = 0;
			//调用一级的尝试获取
			start_free = (char*)malloc_Alloc::allocate(bytes_toget);
		}
		heap_size += bytes_toget;
		end_free = start_free + bytes_toget;
		return (chunk_alloc(size, nobjs));
	}
}

