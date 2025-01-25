#pragma once

#include <Urho3D/Container/Vector.h>

using namespace Urho3D;


template< typename T >
class Stack
{
public:
	Stack() { ; }
	Stack(const size_t size) : storage_(size), pointer_(0) { ; }
    void Reserve(const size_t size)
    {
        storage_.Reserve(size);
    }
	void Resize(const size_t size)
	{
		storage_.Resize(size);
	}
	void Clear()
	{
        pointer_ = 0;
    }
	bool Pop(T& data)
	{
        if (pointer_ > 0)
        {
            data = storage_[pointer_];
            pointer_--;
            return true;
        }
        else
        {
            return false;
        }
	}
	bool Push(const T& data)
	{
        if (pointer_ < Size() - 1)
        {
            pointer_++;
            storage_[pointer_] = data;
            return true;
        }
        else
        {
            return false;
        }
	}
    bool IsEmpty() const { return pointer_ == 0; }

    const size_t Size() const
    {
        return storage_.Size();
    }

private:
	Vector< T > storage_;
	unsigned pointer_;
};

/// if using SetBufferValue, T must be a type POD (due to memset)
template< typename T >
class Matrix2D
{
public:
	Matrix2D() : width_(0), height_(0) { ; }
    Matrix2D(unsigned width, unsigned height)
        : width_(width), height_(height), storage_(width*height) { ; }

	void Resize(unsigned width, unsigned height)
	{
		width_ = width;
		height_ = height;
		storage_.Resize(width*height);
	}

	void Clear()
	{
		width_ = height_ = 0;
		storage_.Clear();
	}

    T& operator()( unsigned x, unsigned y )
    {
        return storage_[ y * width_ + x ];
    }

    const T& operator()( unsigned x, unsigned y ) const
    {
        return storage_[ y * width_ + x ];
    }

    T& operator[]( unsigned addr )
    {
        return storage_[ addr ];
    }

    const T& operator[]( unsigned addr ) const
    {
        return storage_[ addr ];
    }

    void SetBufferFrom(const T* buffer) { if (Size()) memcpy(Buffer(), buffer, sizeof(T) * Size()); }
    void SetBufferValue(unsigned value) { if (Size()) memset(Buffer(), value, sizeof(T) * Size()); }
    void SetBuffer(const T& value) { for (size_t i=0; i < Size(); i++) { storage_[i] = value; } }

    const size_t Height() const { return height_; }
    const size_t Width() const { return width_; }
    const size_t Size() const { return storage_.Size(); }
    T* Buffer() { return &storage_[0]; }

private:
    Vector< T > storage_;
    size_t width_;
    size_t height_;
};

/// To Use only with T = POD type
template< typename T >
class StaticMatrix2D
{
public:
    StaticMatrix2D(unsigned width, unsigned height, T* buffer)
        : width_(width), height_(height), buffer_(buffer) {}

    T& operator()(unsigned x, unsigned y)
    {
        return buffer_[ y * width_ + x ];
    }

    const T& operator()(unsigned x, unsigned y) const
    {
        return buffer_[ y * width_ + x ];
    }

    T& operator[](unsigned addr)
    {
        return buffer_[ addr ];
    }

    const T& operator[](unsigned addr) const
    {
        return buffer_[ addr ];
    }

    const size_t Height() const { return height_; }
    const size_t Width() const { return width_; }
    const size_t Size() const { return width_*height_; }
    T* Buffer() { return buffer_; }

private:
    T* buffer_;
    size_t width_;
    size_t height_;
};
