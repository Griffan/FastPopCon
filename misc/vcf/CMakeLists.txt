cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fpic -Wno-null-conversion")

include_directories(${STATGEN_INCLUDE_DIR})

set(libvcf_SOURCES VcfFile.cpp  VcfFileWriter.cpp     VcfGenotypeFormat.cpp  VcfHeader.cpp  VcfRecord.cpp
        VcfRecordField.cpp   VcfRecordGenotype.cpp  VcfSubsetSamples.cpp VcfFileReader.cpp  VcfGenotypeField.cpp
        VcfGenotypeSample.cpp  VcfHelper.cpp  VcfRecordDiscardRules.cpp  VcfRecordFilter.cpp  VcfRecordInfo.cpp)
add_library(vcf ${libvcf_SOURCES})

