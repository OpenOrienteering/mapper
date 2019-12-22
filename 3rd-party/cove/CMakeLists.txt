project(Cove)

find_package(Qt5Core ${Qt5Core_VERSION} REQUIRED)
find_package(Qt5Widgets ${Qt5Core_VERSION} REQUIRED)
find_package(Qt5Concurrent ${Qt5Core_VERSION} REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("../../src")

set(Cove_SRCS
    app/classificationconfigform.h
    app/colorseditform.h
    app/coverunner.h
    app/FIRFilter.h
    app/mainform.h
    app/Polygons.h
    app/QImageView.h
    app/QPolygonsView.h
    app/Settings.h
    app/Thread.h
    app/UIProgressDialog.h
    app/vectorizationconfigform.h
    libvectorizer/AlphaGetter.h
    libvectorizer/KohonenMap.h
    libvectorizer/MapColor.h
    libvectorizer/Morphology.h
    libvectorizer/PatternGetter.h
    libvectorizer/ParallelImageProcessing.h
    app/classificationconfigform.cpp
    app/colorseditform.cpp
    app/coverunner.cpp
    app/FIRFilter.cpp
    app/mainform.cpp
    app/Polygons.cpp
    app/QImageView.cpp
    app/QPolygonsView.cpp
    app/Settings.cpp
    app/Thread.cpp
    app/UIProgressDialog.cpp
    app/vectorizationconfigform.cpp
    libvectorizer/AlphaGetter.cpp
    libvectorizer/KohonenMap.cpp
    libvectorizer/MapColor.cpp
    libvectorizer/Morphology.cpp
    libvectorizer/PatternGetter.cpp
    libvectorizer/Vectorizer.cpp
    potrace/curve.c
    potrace/potracelib.c
    potrace/trace.c
)

add_library(cove STATIC ${Cove_SRCS})
target_compile_definitions(cove INTERFACE WITH_COVE)
target_include_directories(cove INTERFACE AFTER "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cove Qt5::Widgets Qt5::Concurrent)

add_subdirectory("tests")
