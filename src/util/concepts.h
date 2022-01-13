/**
 * @file concepts.h
 * @author Leonardo Galli (leonardo.galli@bluewin.ch)
 * @brief Defines concepts used throughout the project. Also defines some concepts that will be included in std at some point.
 * @version 0.1
 * @date 2022-01-13
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef __UTIL_CONCEPTS_H
#define __UTIL_CONCEPTS_H

#include <concepts>
#include <type_traits>

template <typename From, typename To>
concept convertible_to = std::is_convertible_v<From, To>;

template<typename From, typename To>
concept same_as = std::is_same_v<From, To>;

template<typename T>
concept integral = std::is_integral_v<T>;

template<typename T>
concept MyContainer = requires {
    typename T::value_type;
    integral<typename T::size_type>;
} and requires(T container, typename T::value_type elem, typename T::size_type num) {
    {T()} -> same_as<T>;

    {container.size()} -> same_as<typename T::size_type>;
    {container.max_size()} -> same_as<typename T::size_type>;
    {container.empty()} -> convertible_to<bool>;
    // can resize to allocate more storage!
    {container.resize(num)};
    // can get data pointer
    {container.data()} -> same_as<typename T::value_type*>;
    // can push_back
    {container.push_back(elem)};
    // can get elem
    {container.at(num)} -> same_as<typename T::value_type&>;
};

template<typename T>
concept POD = std::is_pod_v<T>;

#endif /* __UTIL_CONCEPTS_H */