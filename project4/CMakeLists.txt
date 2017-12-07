cmake_minimum_required(VERSION 3.9)
project(project4)

set(CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_COMPILER, "/usr/local/Cellar/gcc@5/5.4.0_1")

add_library(LIBRBF
        src/rbf/pfm.cc
        src/rbf/pfm.h
        src/rbf/rbfm.cc
        src/rbf/rbfm.h
        src/rbf/test_util.h
        src/rm/rm.h
        src/rm/rm.cc
        src/rm/rm_test_util.h
        src/ix/ix.h
        src/ix/ix.cc
        src/ix/ix_test_util.h
        src/qe/qe_test_util.h
        src/qe/qe.h
        src/qe/qe.cc)

add_executable(rbftest1 src/rbf/rbftest1.cc)
target_link_libraries(rbftest1 LIBRBF)

add_executable(rbftest2 src/rbf/rbftest2.cc)
target_link_libraries(rbftest2 LIBRBF)

add_executable(rbftest3 src/rbf/rbftest3.cc)
target_link_libraries(rbftest3 LIBRBF)

add_executable(rbftest4 src/rbf/rbftest4.cc)
target_link_libraries(rbftest4 LIBRBF)

add_executable(rbftest5 src/rbf/rbftest5.cc)
target_link_libraries(rbftest5 LIBRBF)

add_executable(rbftest6 src/rbf/rbftest6.cc)
target_link_libraries(rbftest6 LIBRBF)

add_executable(rbftest7 src/rbf/rbftest7.cc)
target_link_libraries(rbftest7 LIBRBF)

add_executable(rbftest8 src/rbf/rbftest8.cc)
target_link_libraries(rbftest8 LIBRBF)

add_executable(rbftest8b src/rbf/rbftest8b.cc)
target_link_libraries(rbftest8b LIBRBF)

add_executable(rbftest9 src/rbf/rbftest9.cc)
target_link_libraries(rbftest9 LIBRBF)

add_executable(rbftest10 src/rbf/rbftest10.cc)
target_link_libraries(rbftest10 LIBRBF)

add_executable(rbftest11 src/rbf/rbftest11.cc)
target_link_libraries(rbftest11 LIBRBF)

add_executable(rbftest12 src/rbf/rbftest12.cc)
target_link_libraries(rbftest12 LIBRBF)

add_executable(rbftest_delete src/rbf/rbftest_delete.cc)
target_link_libraries(rbftest_delete LIBRBF)

add_executable(rbftest_update src/rbf/rbftest_update.cc)
target_link_libraries(rbftest_update LIBRBF)

add_executable(rmtest_00 src/rm/rmtest_00.cc)
target_link_libraries(rmtest_00 LIBRBF)

add_executable(rmtest_01 src/rm/rmtest_01.cc)
target_link_libraries(rmtest_01 LIBRBF)

add_executable(rmtest_02 src/rm/rmtest_02.cc)
target_link_libraries(rmtest_02 LIBRBF)

add_executable(rmtest_03 src/rm/rmtest_03.cc)
target_link_libraries(rmtest_03 LIBRBF)

add_executable(rmtest_04 src/rm/rmtest_04.cc)
target_link_libraries(rmtest_04 LIBRBF)

add_executable(rmtest_05 src/rm/rmtest_05.cc)
target_link_libraries(rmtest_05 LIBRBF)

add_executable(rmtest_06 src/rm/rmtest_06.cc)
target_link_libraries(rmtest_06 LIBRBF)

add_executable(rmtest_07 src/rm/rmtest_07.cc)
target_link_libraries(rmtest_07 LIBRBF)

add_executable(rmtest_08 src/rm/rmtest_08.cc)
target_link_libraries(rmtest_08 LIBRBF)

add_executable(rmtest_09 src/rm/rmtest_09.cc)
target_link_libraries(rmtest_09 LIBRBF)

add_executable(rmtest_10 src/rm/rmtest_10.cc)
target_link_libraries(rmtest_10 LIBRBF)

add_executable(rmtest_11 src/rm/rmtest_11.cc)
target_link_libraries(rmtest_11 LIBRBF)

add_executable(rmtest_12 src/rm/rmtest_12.cc)
target_link_libraries(rmtest_12 LIBRBF)

add_executable(rmtest_13 src/rm/rmtest_13.cc)
target_link_libraries(rmtest_13 LIBRBF)

add_executable(rmtest_13b src/rm/rmtest_13b.cc)
target_link_libraries(rmtest_13b LIBRBF)

add_executable(rmtest_14 src/rm/rmtest_14.cc)
target_link_libraries(rmtest_14 LIBRBF)

add_executable(rmtest_15 src/rm/rmtest_15.cc)
target_link_libraries(rmtest_15 LIBRBF)

add_executable(rmtest_create_tables src/rm/rmtest_create_tables.cc)
target_link_libraries(rmtest_create_tables LIBRBF)

add_executable(rmtest_delete_tables src/rm/rmtest_delete_tables.cc)
target_link_libraries(rmtest_delete_tables LIBRBF)

add_executable(ixtest_01 src/ix/ixtest_01.cc)
target_link_libraries(ixtest_01 LIBRBF)

add_executable(ixtest_02 src/ix/ixtest_02.cc)
target_link_libraries(ixtest_02 LIBRBF)

add_executable(ixtest_03 src/ix/ixtest_03.cc)
target_link_libraries(ixtest_03 LIBRBF)

add_executable(ixtest_04 src/ix/ixtest_04.cc)
target_link_libraries(ixtest_04 LIBRBF)

add_executable(ixtest_05 src/ix/ixtest_05.cc)
target_link_libraries(ixtest_05 LIBRBF)

add_executable(ixtest_06 src/ix/ixtest_06.cc)
target_link_libraries(ixtest_06 LIBRBF)

add_executable(ixtest_07 src/ix/ixtest_07.cc)
target_link_libraries(ixtest_07 LIBRBF)

add_executable(ixtest_08 src/ix/ixtest_08.cc)
target_link_libraries(ixtest_08 LIBRBF)

add_executable(ixtest_09 src/ix/ixtest_09.cc)
target_link_libraries(ixtest_09 LIBRBF)

add_executable(ixtest_10 src/ix/ixtest_10.cc)
target_link_libraries(ixtest_10 LIBRBF)

add_executable(ixtest_11 src/ix/ixtest_11.cc)
target_link_libraries(ixtest_11 LIBRBF)

add_executable(ixtest_12 src/ix/ixtest_12.cc)
target_link_libraries(ixtest_12 LIBRBF)

add_executable(ixtest_13 src/ix/ixtest_13.cc)
target_link_libraries(ixtest_13 LIBRBF)

add_executable(ixtest_14 src/ix/ixtest_14.cc)
target_link_libraries(ixtest_14 LIBRBF)

add_executable(ixtest_15 src/ix/ixtest_15.cc)
target_link_libraries(ixtest_15 LIBRBF)

add_executable(ixtest_p1 src/ix/ixtest_p1.cc)
target_link_libraries(ixtest_p1 LIBRBF)

add_executable(ixtest_p2 src/ix/ixtest_p2.cc)
target_link_libraries(ixtest_p2 LIBRBF)

add_executable(ixtest_p3 src/ix/ixtest_p3.cc)
target_link_libraries(ixtest_p3 LIBRBF)

add_executable(ixtest_p4 src/ix/ixtest_p4.cc)
target_link_libraries(ixtest_p4 LIBRBF)

add_executable(ixtest_p5 src/ix/ixtest_p5.cc)
target_link_libraries(ixtest_p5 LIBRBF)

add_executable(ixtest_p6 src/ix/ixtest_p6.cc)
target_link_libraries(ixtest_p6 LIBRBF)

add_executable(ixtest_extra_01 src/ix/ixtest_extra_01.cc)
target_link_libraries(ixtest_extra_01 LIBRBF)

add_executable(ixtest_extra_02 src/ix/ixtest_extra_02.cc)
target_link_libraries(ixtest_extra_02 LIBRBF)

add_executable(ixtest_pe_01 src/ix/ixtest_pe_01.cc)
target_link_libraries(ixtest_pe_01 LIBRBF)

add_executable(ixtest_pe_02 src/ix/ixtest_pe_02.cc)
target_link_libraries(ixtest_pe_02 LIBRBF)

add_executable(qetest_01 src/qe/qetest_01.cc)
target_link_libraries(qetest_01 LIBRBF)
