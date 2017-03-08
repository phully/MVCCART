/*
 * types.hpp
 *
 *  Created on: Jun 5, 2014
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_TYPES_HPP_
#define LIBCPP_TYPES_TYPES_HPP_


/// include all types

////// smart pointers
#include "detail/Function.hpp"
#include "detail/SharedPtr.hpp"
#include "detail/SharedInstance.hpp"
#include "detail/IntrusivePtr.hpp"
#include "detail/UniquePtr.hpp"
#include "detail/UniqueInstance.hpp"

////// misc
#include "detail/TupleType.hpp"
#include "detail/SubstringRef.hpp"


/// include all type traits

#include "traits/PointerTraits.hpp"
#include "traits/GetPointedElementType.hpp"
#include "traits/FunctionTraits.hpp"

#endif /* LIBCPP_TYPES_TYPES_HPP_ */
