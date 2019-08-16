// Copyright (C) 2019 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/stl_interfaces/container_interface.hpp>

#include <algorithm>
#include <iterator>
#include <memory>

#include <cassert>


//[ static_vector_defn
// The sections of member functions below are marked as they are in the
// standard for std::vector.  Each section has two numbers: the number of
// member functions in that section, and the number that are missing, because
// they are provided by container_interface.  The purely allocator-specific
// members are neither present nor part of the count.
template<typename T, std::size_t N>
struct static_vector : boost::stl_interfaces::container_interface<
                           static_vector<T, N>,
                           boost::stl_interfaces::contiguous>
{
    // These are the types required for reversible containers.  These must be
    // user-defined.
    using value_type = T;
    using pointer = T *;
    using const_pointer = T const *;
    using reference = value_type &;
    using const_reference = value_type const &;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = T *;
    using const_iterator = T const *;
    using reverse_iterator = boost::stl_interfaces::reverse_iterator<iterator>;
    using const_reverse_iterator =
        boost::stl_interfaces::reverse_iterator<const_iterator>;

    // construct/copy/destroy (9 members, skipped 2)
    //
    // Constructors all must be user-provided.  Assignment from
    // std::initializer_list and the destructor come from container_interfacey.
    static_vector() noexcept : size_(0) {}
    explicit static_vector(size_type n) : size_(0)
    {
        // Note that you must write "this->" before all the member functions
        // provided by container_interface, which is slightly annoying.
        this->assign(n, T());
    }
    explicit static_vector(size_type n, T const & x) : size_(0)
    {
        this->assign(n, x);
    }
    template<
        typename InputIterator,
        typename Enable = std::enable_if_t<std::is_convertible<
            typename std::iterator_traits<InputIterator>::iterator_category,
            std::input_iterator_tag>::value>>
    static_vector(InputIterator first, InputIterator last) : size_(0)
    {
        this->assign(first, last);
    }
    static_vector(std::initializer_list<T> il) :
        static_vector(il.begin(), il.end())
    {}
    static_vector(static_vector const & other) : size_(0)
    {
        this->assign(other.begin(), other.end());
    }
    static_vector(static_vector && other) noexcept(
        noexcept(std::declval<static_vector>().emplace_back(
            std::move(*other.begin())))) :
        size_(0)
    {
        for (auto & element : other) {
            emplace_back(std::move(element));
        }
        other.clear();
    }
    static_vector & operator=(static_vector const & other)
    {
        this->clear();
        this->assign(other.begin(), other.end());
        return *this;
    }
    static_vector & operator=(static_vector && other) noexcept(noexcept(
        std::declval<static_vector>().emplace_back(std::move(*other.begin()))))
    {
        this->clear();
        for (auto & element : other) {
            emplace_back(std::move(element));
        }
        other.clear();
        return *this;
    }

    // iterators (2 members, skipped 10)
    //
    // This section is the first big win.  Instead of having to write 12
    // overloads line begin, cbegin, rbegin, crbegin, etc., we can just write
    // 2.
    iterator begin() noexcept { return reinterpret_cast<T *>(buf_); }
    iterator end() noexcept
    {
        return reinterpret_cast<T *>(buf_ + size_ * sizeof(T));
    }

    // capacity (5 members, skipped 3)
    //
    // Most of these are not even part of the general requirements, because
    // some are specific to std::vector and related types.  However, we do get
    // empty, size, and the unary overload of resize from container_interface.
    size_type max_size() const noexcept { return N; }
    size_type capacity() const noexcept { return N; }
    void resize(size_type sz, T const & x) noexcept
    {
        assert(sz < capacity());
        if (sz < this->size())
            erase(begin() + sz, end());
        if (this->size() < sz)
            std::uninitialized_fill(end(), begin() + sz, x);
        size_ = sz;
    }
    void reserve(size_type n) noexcept { assert(n < capacity()); }
    void shrink_to_fit() noexcept {}

    // element access (skipped 8)
    // data access (skipped 2)
    //
    // Another big win.  container_interface provides all of the overloads of
    // operator[], at, front, back, and data.

    // modifiers (5 members, skipped 9)
    //
    // In this section we again get most of the API from container_interface.

    // emplace_back does not look very necessary -- just look at its trivial
    // implementation -- but we can't provide it from container_interface,
    // because it is an optional sequence container interface.  We would not
    // want emplace_front to suddenly appear on our std::vector-like type, and
    // there may be some other type for which emplace_back is a bad idea.
    //
    // However, by providing emplace_back here, we signal to the
    // container_interface template that our container is
    // back-mutation-friendly, and this allows it to provide all the overloads
    // of push_back, and pop_back.
    template<typename... Args>
    reference emplace_back(Args &&... args)
    {
        return *emplace(end(), std::forward<Args>(args)...);
    }
    template<typename... Args>
    iterator emplace(const_iterator pos, Args &&... args)
    {
        auto position = const_cast<T *>(pos);
        bool const insert_before_end = position < end();
        if (insert_before_end) {
            auto last = end();
            emplace_back(std::move(this->back()));
            std::move_backward(position, last - 1, last);
        }
        new (position) T(std::forward<Args>(args)...);
        if (!insert_before_end)
            ++size_;
        return position;
    }
    // Note: The iterator category here was upgraded to ForwardIterator
    // (instead of vector's InputIterator), to ensure linear time complexity.
    template<
        typename ForwardIterator,
        typename Enable = std::enable_if_t<std::is_convertible<
            typename std::iterator_traits<ForwardIterator>::iterator_category,
            std::forward_iterator_tag>::value>>
    iterator
    insert(const_iterator pos, ForwardIterator first, ForwardIterator last)
    {
        auto position = const_cast<T *>(pos);
        auto const insertions = std::distance(first, last);
        assert(this->size() + insertions < capacity());
        std::uninitialized_fill_n(end(), insertions, T());
        std::move_backward(position, end(), end() + insertions);
        std::copy(first, last, position);
        size_ += insertions;
        return position;
    }
    iterator erase(const_iterator f, const_iterator last)
    {
        auto first = const_cast<T *>(f);
        auto end_ = this->cend();
        auto it = std::move(last, end_, first);
        for (; it != end_; ++it) {
            it->~T();
        }
        size_ -= last - first;
        return first;
    }
    void swap(static_vector & other)
    {
        size_type short_size, long_size;
        std::tie(short_size, long_size) =
            std::minmax(this->size(), other.size());
        for (auto i = size_type(0); i < short_size; ++i) {
            using std::swap;
            swap((*this)[i], other[i]);
        }

        static_vector * longer = this;
        static_vector * shorter = this;
        if (this->size() < other.size())
            longer = &other;
        else
            shorter = &other;

        for (auto it = longer->begin() + short_size, last = longer->end();
             it != last;
             ++it) {
            shorter->emplace_back(std::move(*it));
        }

        longer->resize(short_size);
        shorter->size_ = long_size;
    }

    // Since we're getting so many overloads from container_interface, and
    // since many of those overloads are implemented in terms of a
    // user-defined function of the same name, we need to add quite a few
    // using declarations here.
    using base_type = boost::stl_interfaces::container_interface<
        static_vector<T, N>,
        boost::stl_interfaces::contiguous>;
    using base_type::begin;
    using base_type::end;
    using base_type::resize;
    using base_type::insert;
    using base_type::erase;

    // comparisons (2 free functions, skipped 4)
    friend bool operator==(static_vector const & lhs, static_vector const & rhs)
    {
        return lhs.size() == rhs.size() &&
               std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
    friend bool operator<(static_vector const & lhs, static_vector const & rhs)
    {
        auto const iters =
            std::mismatch(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        if (iters.second == rhs.end())
            return false;
        if (iters.first == lhs.end())
            return true;
        return *iters.first < *iters.second;
    }

private:
    alignas(T) unsigned char buf_[N * sizeof(T)];
    size_type size_;
};
//]