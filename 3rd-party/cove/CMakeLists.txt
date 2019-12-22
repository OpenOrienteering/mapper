project(Cove)

find_package(Qt5Core ${Qt5Core_VERSION} REQUIRED)
find_package(Qt5Widgets ${Qt5Core_VERSION} REQUIRED)
find_package(Qt5Concurrent ${Qt5Core_VERSION} REQUIRED)

set(CMAKE_CXX_STANDARD 14)

add_library(cove-potrace STATIC
    potrace/curve.c
    potrace/potracelib.c
    potrace/trace.c
)
target_include_directories(cove-potrace INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(cove-vectorizer STATIC
    libvectorizer/AlphaGetter.cpp
    libvectorizer/ClassificationThread.cpp
    libvectorizer/KohonenMap.cpp
    libvectorizer/MapColor.cpp
    libvectorizer/Morphology.cpp
    libvectorizer/PatternGetter.cpp
    libvectorizer/Polygons.cpp
    libvectorizer/Vectorizer.cpp
    libvectorizer/ParallelImageProcessing.h
)
target_include_directories(cove-vectorizer
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/libvectorizer"
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(cove-vectorizer PRIVATE cove-potrace)
target_link_libraries(cove-vectorizer PUBLIC Qt5::Gui Qt5::Concurrent)

set(Cove_SRCS
    app/classificationconfigform.cpp
    app/colorseditform.cpp
    app/coverunner.cpp
    app/FIRFilter.cpp
    app/mainform.cpp
    app/QImageView.cpp
    app/QPolygonsView.cpp
    app/Settings.cpp
    app/UIProgressDialog.cpp
    app/vectorizationconfigform.cpp
)

add_library(cove STATIC ${Cove_SRCS})
target_compile_definitions(cove INTERFACE WITH_COVE)
target_include_directories(cove
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/app"
    "${Mapper_SOURCE_DIR}/src"
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(cove cove-vectorizer Qt5::Widgets)
set_target_properties(cove
  PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
)

add_subdirectory("tests")
