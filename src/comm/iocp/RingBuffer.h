
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

static const int _size = 256*10240;

// �����ֽ����Ļ��λ�����
template <class T>
class CRingBuffer
{
public:
	CRingBuffer(unsigned long size = 256);
	~CRingBuffer();
public:
	unsigned long size();
	unsigned long space();
	bool put(T *data, int len);
	bool get(T *data, int len, bool takeAway);
	bool peek(T *data, int len);
    bool seek(int len); //
private:
	T* _rb;
	unsigned long _first;			// ָ���һ������λ��
	unsigned long _last;			// ָ���һ��д��λ��
	unsigned long _size;
};

template <class T>
CRingBuffer<T>::CRingBuffer(unsigned long size/* = 256*/)
{
	_first = 0;
	_last = 0;
	_size = size;
	_rb = new T[_size];
    ZeroMemory(_rb,_size*sizeof(T));
}

template <class T>
CRingBuffer<T>::~CRingBuffer()
{
  delete []_rb;
  _rb = 0;
}

// ���ݳ���
template <class T>
unsigned long CRingBuffer<T>::size()
{
	if((0 <= _first && _first < _size) && (0 <= _last && _last < _size))
	{

		if(_last >= _first)
			return _last - _first;
		else
			return _size - _first + _last;
	}
	else
		return 0;
}

// ʣ��ռ�
template <class T>
unsigned long CRingBuffer<T>::space()//���ζ�����ʣ��ռ�
{
	return _size - 1 - size();
}

// ��������д����
template <class T>
bool CRingBuffer<T>::put(T *data, int len)
{

	if(data == NULL || len <=0)
		return false;
	
	if(len > (int)space()) // ��֤���㹻�Ŀռ�
		return false;
	
	if(_size - _last >= (unsigned long)len) // ������ĩβ���㹻�ռ�
	{
		memcpy(_rb + _last, data, len*sizeof(T));
		_last = (_last + len) % _size;
	}
	else // ������ĩβ�ռ䲻���������θ���
	{
		memcpy(_rb + _last, data, (_size - _last)*sizeof(T));
		memcpy(_rb, data + _size - _last, (len - (_size - _last))*sizeof(T));
		_last = len - (_size - _last);
	}
	return true;
}

// �ӻ�����������
template <class T>
bool CRingBuffer<T>::get(T *data, int len, bool takeAway = true)
{
	if(data == NULL || len <=0)
		return false;
	
	if(len > (int)size()) // ��֤���㹻������
		return false;
	
	if(_last >= _first || _size - _first >= (unsigned long)len) // ������������������ĩβ���㹻����
	{
		memcpy(data, _rb + _first, len*sizeof(T));
		if(takeAway)
			_first = (_first + len) % _size;
	}
	else // ���ݲ���������ĩβû���㹻���ݣ������θ���
	{
		memcpy(data, _rb + _first, (_size - _first)*sizeof(T));
		memcpy(data + _size - _first, _rb, (len - (_size - _first))*sizeof(T));
		if(takeAway)
			_first = len - (_size - _first);
	}
	return true;
}

// �鿴����������ȡ��
template <class T>
bool CRingBuffer<T>::peek(T *data, int len)
{
	return get(data, len, false);
}

template <class T>
bool CRingBuffer<T>::seek(int len)
{
	if(len <= 0)
		return false;
	
	if(len > (int)size()) // ��֤���㹻������
		return false;
	
	if(_last >= _first || _size - _first >= (unsigned long)len) // ������������������ĩβ���㹻����
		_first = (_first + len) % _size;
	else // ���ݲ���������ĩβû���㹻���ݣ������θ���
		_first = len - (_size - _first);
	
	return true;
}

#endif // #ifndef RING_BUFFER_H