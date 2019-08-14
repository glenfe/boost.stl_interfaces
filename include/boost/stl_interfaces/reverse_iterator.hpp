// Copyright (C) 2019 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_STL_INTERFACES_REVERSE_ITERATOR_HPP
#define BOOST_STL_INTERFACES_REVERSE_ITERATOR_HPP

#include <boost/stl_interfaces/fwd.hpp>


namespace boost { namespace stl_interfaces {

    // TODO: Tests.
    // TODO: Look in all the headers for ADL traps.

    /** This type is very similar to the C++20 version of
        `std::reverse_iterator`; it is `constexpr`-, `noexcept`-, and
        proxy-friendly. */
    template<typename BidiIter>
    struct reverse_iterator
        : boost::stl_interfaces::iterator_interface<
              reverse_iterator<BidiIter>,
#if 201703L < __cplusplus && defined(__cpp_lib_ranges)
              typename std::iterator_traits<BidiIter>::iterator_concept,
#else
              typename std::iterator_traits<BidiIter>::iterator_category,
#endif
              typename std::iterator_traits<BidiIter>::value_type,
              typename std::iterator_traits<BidiIter>::reference,
              typename std::iterator_traits<BidiIter>::pointer,
              typename std::iterator_traits<BidiIter>::difference_type>
    {
        constexpr reverse_iterator() noexcept(noexcept(BidiIter())) : it_() {}
        constexpr reverse_iterator(BidiIter it) noexcept(
            noexcept(BidiIter(it))) :
            it_(it)
        {}

        constexpr typename std::iterator_traits<BidiIter>::reference
        operator*() const
            noexcept(noexcept(std::prev(std::declval<BidiIter>())))
        {
            return *std::prev(it_);
        }

        constexpr reverse_iterator & operator+=(
            typename std::iterator_traits<BidiIter>::difference_type
                n) noexcept(noexcept(std::advance(std::declval<BidiIter>(), -n)))
        {
            std::advance(it_, -n);
            return *this;
        }

    private:
        friend boost::stl_interfaces::access;
        constexpr BidiIter & base_reference() noexcept { return it_; }
        constexpr BidiIter const & base_reference() const noexcept
        {
            return it_;
        }

        BidiIter it_;
    };

    /** Makes a `reverse_iterator<BidiIter>` from an iterator of type
        `Bidiiter`. */
    template<typename BidiIter>
    auto make_reverse_iterator(BidiIter it)
    {
        return reverse_iterator<BidiIter>(it);
    }

}}

#endif