/*

CPP-Proxy-Ptr is licensed under MIT licence

Author: StarBrilliant https://github.com/m13253

Copyright (c) 2015

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef PROXY_PTR_HPP
#define PROXY_PTR_HPP

#include <memory>

template<typename _Tp, typename Allocator = std::allocator<_Tp>>
class proxy_ptr {

public:

    typedef _Tp *pointer;
    typedef _Tp element_type;

    template<typename ...Args>
    explicit proxy_ptr(Args &&...args) :
        _alloc(Allocator()),
        _ptr(_alloc.allocate(1)) {
        try {
            _alloc.construct(_ptr, std::forward<Args>(args)...);
        } catch(...) {
            this->~proxy_ptr();
            throw;
        }
    }

    proxy_ptr(const proxy_ptr &other) :
        _alloc(Allocator()),
        _ptr(_alloc.allocate(1)) {
        try {
            _alloc.construct(_ptr, *other._ptr);
        } catch(...) {
            this->~proxy_ptr();
            throw;
        }
    }

    proxy_ptr(proxy_ptr &&other) :
        _ptr(other._ptr) {
        other._ptr = nullptr;
    }

    proxy_ptr(const element_type &value) :
        _alloc(Allocator()),
        _ptr(_alloc.allocate(1)) {
        try {
            _alloc.construct(_ptr, value);
        } catch(...) {
            this->~proxy_ptr();
            throw;
        }
    }

    proxy_ptr(element_type &&value) :
        _alloc(Allocator()),
        _ptr(_alloc.allocate(1)) {
        try {
            _alloc.construct(_ptr, std::move(value));
        } catch(...) {
            this->~proxy_ptr();
            throw;
        }
    }

    ~proxy_ptr() {
        if(_ptr) {
            _alloc.destroy(_ptr);
            _alloc.deallocate(_ptr, 1);
            _ptr = nullptr;
        }
    }

    proxy_ptr &operator=(const proxy_ptr &that) {
        *_ptr = *that._ptr;
        return *this;
    }

    proxy_ptr &operator=(proxy_ptr &&that) noexcept {
        std::swap(_ptr, that._ptr);
        return *this;
    }

    proxy_ptr &operator=(const element_type &value) {
        *_ptr = value;
    }

    proxy_ptr &operator=(element_type &&value) {
        *_ptr = std::move(value);
    }

    element_type &operator*() const noexcept {
        return _ptr;
    }

    pointer operator->() const noexcept {
        return _ptr;
    }

    explicit operator pointer() const noexcept {
        return _ptr;
    }

    pointer get() const noexcept {
        return _ptr;
    }

    void swap(proxy_ptr &that) noexcept {
        std::swap(_ptr, that._ptr);
    }

    void swap_payload(proxy_ptr &that) {
        std::swap(*_ptr, *that._ptr);
    }

private:

    Allocator _alloc;
    pointer _ptr;

};

#endif
