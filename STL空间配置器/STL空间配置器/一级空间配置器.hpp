#pragma once
#include <iostream>
using namespace std;

//���û������ڴ����128ʱʹ��һ���ռ�������
template <int inst>
class malloc_Alloc_template {   
public:
	static void *allocate(size_t n)
	{
		void *result = malloc(n);
		if (n == 0)
			result = oom_malloc(n); //������Ҫ����д���
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


	static void(*set_malloc_handler(void(*f)()))()//�û������Լ��趨
		                                         //û���ڴ�ʱ���·���ĺ���
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
void(*malloc_Alloc_template<inst>::alloc_oom_handler)() = NULL; //��̬��Ա��ʼ��

typedef malloc_Alloc_template<0> malloc_Alloc;

//�����ռ�������

enum { ALIGN = 8 }; //С�����Ͻ�
enum { MAX_BYTES = 128 };  //��������
enum { FREELIST = MAX_BYTES / ALIGN }; //С�������

template <int inst>
class default_Alloc_template {
public:
	static void *allocate(size_t n);
	static void *reallocate(void *p, size_t old_size, size_t new_size);
	static void deallocate(void *p, size_t n);
private:
	//�������������ڴ��С��С�����Ͻ�ı���
	static size_t ROUND_UP(size_t bytes)
	{
		return ((bytes + ALIGN - 1) & ~(ALIGN - 1));
	}
	//�±�
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
	static char* start_free; //�ڴ����ʼλ��
	static char* end_free;  //�ڴ�ؽ���λ��  
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
	// ����128����һ���ռ�������
	if (n > (size_t)MAX_BYTES)
	{
		return (malloc_Alloc::allocate(n));
	}
	//�ж�Ӧ��ʹ��my_free_listӦ��ʹ��16���е���һ��
	my_free_list = free_list + FREELIST_INDEX(n);
	result = *my_free_list;

	//û���ҵ���˵��û������Ҫ��free_list
	if (0 == result)
	{
		void *r = refill(ROUND_UP(n));
		return r;
	}
	//������Ҫ��free_list,free_listָ����һ�� 
	*my_free_list = result->free_list_link;
	return result;
}

template <int inst>
void default_Alloc_template<inst>::deallocate(void *p, size_t n)
{
	obj *q = (obj *)p;
	obj * volatile *my_free_list;
	//����128����1���ռ��������ĺ���
	if (n > (size_t)FREELIST)
	{
		malloc_Alloc::dellocate(p, n);
		return;
	}
	my_free_list = free_list + FREELIST_INDEX(n);
	q->free_list_link = *my_free_list;
	*my_free_list = q;
}

//freelistû�п��õ�����ʱ������refill����¿ռ�
template <int inst>
void* default_Alloc_template<inst>::refill(size_t n)
{
	//Ĭ��ȡ��20������
	int nobjs = 20;
	//���ڴ��ȡ�������free_listʹ��
	char *chunk = chunk_alloc(n, nobjs);
	obj* volatile *my_free_list;
	obj* result;
	obj* cur_obj;
	obj* next_obj;
	int i;
	//ֻȡ����һ����ֱ�ӽ���������
	if (1 == nobjs)
		return chunk;
	my_free_list = free_list + FREELIST_INDEX(n);
	result = (obj*)chunk;
	*my_free_list = next_obj = (obj*)(chunk + n);
	//��ʣ������齻��free_list�����ڵ���������
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
//���ڴ��ȡ�ÿռ佻��free_listʹ��
template <int inst>
char *default_Alloc_template<inst>::chunk_alloc(size_t size, int &nobjs)
{
	char *result;
	size_t total_bytes = size*nobjs; //��Ҫ�Ŀռ�
	size_t bytes_left = end_free - start_free; //�ڴ��ʣ��Ŀռ�
	
	//ʣ��ռ��㹻
	if (bytes_left >= total_bytes)
	{
		result = start_free;
		start_free += total_bytes;
		return (result);
	}
	//ʣ��ռ䲻��������������һ��С����
	else if (bytes_left >= size)
	{
		//�ܹ��õ����������
		nobjs = bytes_left/ size;
		total_bytes = nobjs*size;
		result = start_free;
		start_free += total_bytes;
		return result;
	}
	//ʣ���ڴ��޷��ṩ1����С������
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
		//����heap�ռ䣬���������ڴ��
		start_free = (char*)malloc(bytes_toget);
		if (0 == start_free)
		{
			//heap�ռ䲻�㣬mallocʧ��
			int i;
			obj* volatile *my_free_list;
			obj* p;
			//�Ӹ�������鳢�Ի�ȡ
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
			//����һ���ĳ��Ի�ȡ
			start_free = (char*)malloc_Alloc::allocate(bytes_toget);
		}
		heap_size += bytes_toget;
		end_free = start_free + bytes_toget;
		return (chunk_alloc(size, nobjs));
	}
}

