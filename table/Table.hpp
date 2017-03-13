/*
 * Copyright (c) 2014-17 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#ifndef Table_hpp_
#define Table_hpp_

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <exception>
#include <iterator>
#include <mutex>
#include <shared_mutex>
#include <boost/signals2.hpp>
#include "TableInfo.hpp"
#include "TableException.hpp"
#include "BaseTable.hpp"
#include "../ARTFULCpp.h"


//#define DefaultKeyType char[20]


namespace pfabric {

//template <typename RecordType, typename KeyType = DefaultKeyType>
//using Table = pfabric::HashMapTable<RecordType, KeyType>;

    template <typename RecordType, typename KeyType = char[20]>
    using ARTable = ARTFULCpp<RecordType, KeyType>;

}


#endif

