Forked from http://github.com/armon/libart
C++ wrapping,  C++11 primitives, Boost SmartPointers, threads and tests were used and also pfabric "core/Tuple.hpp"

 Running Solution on multi-core machine cloud01.prakinf.tu-ilmenau.de cloud:

1-  SSH into server with credentials with login id:
    ssh -p 2222  yourlogin@cloud01.prakinf.tu-ilmenau.de
2-  Clone a new MVCCART repository from  Github:
    https://github.com/umairsheikh/MVCCART
3-  Please Delete cmake build files, i.e.. cmake-build-debug, CMakeFiles, cmake_install.cmake, CMakeCache.txt and
    I am sorry for it needed to be ignored, i will do it i promise.

4- Rename CMakeLists_Production to CMakeLists.txt , written for multicore boost environment for prakinf.tu-ilmenau awesome cloud.
  boost install path is hardcoded and should be set accordingly with your boost install path in a file  CMakeFile_Production.txt,
  which is to be renamed ofcourse to make it functional. (Original CMake is for the dev environment i have on MACOSX, It works
  fine and CMakes auto finds boost as well)

5- Update the CMakeLists.txt file, and choose the desired Test CPP file, to run from WorkloadTests or from CorrectnessTests
  folder relative path.

6- In your MVCCART path, and execute command -> Cmake .
- Compile the code through CMAKE using command:
    cmake --build MVCCART/ --target mvccart -- -j 4
- Running the solution  ./mvccart
  ERROR:: "Please set path in your WorkloadTests/...CPP file chosen" to run. I need to automate this too. Path should be
  absolute path to test_data/words.txt" file. Here it it to be updated in a variable "rootpath_words"



NOTE:: Project is a part of Pipefabric. It has only depency on pfabric "core/Tuple.hpp" referenced from ART/ArtCPP.hpp so far. It can be
very easy for pfabricers to setup this project with complete pfabric branch too. Just Make Pfabric branch, and add this repository
under Pfabric branch, and use "Core/Tuple.hpp" from the pfabric complete branch.  However, It can be directly imported
in Clion and readily be used without any extra headaches of pfabric as well, since a limited code from pfabric is already imported
in this repository and depencies were resolved for "core/Tuple.hpp"



Your own Development setup with your own ART Instances can be used as follows:

1- Add following headers, in you pfabric project. I

 #include <iostream>
 #include <boost/test/unit_test.hpp>
 #include <boost/lexical_cast.hpp>
 #include "mvcc/mvcc.hpp"
 #include "ART/ARTFULCpp.h"
 //#include "Transactions/WriteOnlyTemplates.h" Use templates already defined. from Transaction folder.

2- Define globals to test and reuse typdef is handy.

 typedef pfabric::Tuple<string,unsigned long, int,string, double> RecordType;
 typedef char KeyType[20];
 typedef ARTFULCpp<RecordType,KeyType> ARTTupleContainer;
 char KeysToStore[235890][20];
 std::vector<RecordType> vectorValues;


2.1- Tuple schema is defined like: typedef pfabric::Tuple<std::string,unsigned long, int, std::string, double> RecordType;

2.2- test_data/Words.txt file, is imported and all the keys are stored in an array as KeysToStore

2,3- Vector<RecordType> is created, as a process to test and store values, in order to compare the tuple values after
inserting to ART and updating the tuple attributes while updating operation. Here is how tuples are generated.
While index is the iterative index, while reading values from words.txt. Similar to HashMapTableTest.cpp.

RecordType tuple = RecordType(buf,
                             (unsigned long) index,
                             index + 100,
                             fmt::format("String/{}", buf),
                             index / 100.0);

vectorValues.push_back(tuple);


3- Adaptive Radix Tree instance can be used as such: While ARTTupleContainer is defined in globals.
auto ARTable1 =  new ARTTupleContainer();

5- Transactions are created and can be seen directly from here as specimen.
https://github.com/umairsheikh/MVCCART/blob/master/WorkloadTests/
https://github.com/umairsheikh/MVCCART/tree/master/CorrectnessTests
https://github.com/umairsheikh/MVCCART/blob/master/WorkloadTests/



