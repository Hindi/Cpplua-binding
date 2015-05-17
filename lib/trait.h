#ifndef TRAIT_H
#define TRAIT_H

/**
 * Copyright (c) 2014 Domora
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

// Our "tag"
template <std::size_t... Is>
struct _indices {};

// Recursively inherits from itself until...
template <std::size_t N, std::size_t... Is>
struct _indices_builder : _indices_builder<N-1, N-1, Is...> {};

// The base case where we define the type tag
template <std::size_t... Is>
struct _indices_builder<0, Is...> {
    using type = _indices<Is...>;
};

template <typename T> struct _id {};
#endif
