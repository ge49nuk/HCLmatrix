echo "1. Removing old-cmake-files..."
rm -r CMakeCache.txt  ./CMakeFiles cmake_install.cmake  Makefile

echo "2. Exporting MPICH on BEAST ..."
module use ~/localLibs/myModules
module use ~/spack/share/spack/lmod/linux-sles15-zen2
module load mpich-3.3.2-gcc-10.2.1-ooqbcd3

# load dependencies
echo "3. Loading thallium-rpc, mercury, margo dependencies..."
module load hwloc-2.4.1-gcc-10.2.1-bu232a3
module load cmake-3.20.3-gcc-10.2.1-crj7py7
module load argobots-1.1-gcc-10.2.1-jlfvqv6
module load boost-1.76.0-gcc-10.2.1-hotyooi
module load cereal-1.3.0-gcc-10.2.1-vd6dtp3
module load libfabric-1.11.1-gcc-10.2.1-iy4glnm
module load mercury-2.0.1-gcc-10.2.1-5llxbgd
module load mochi-abt-io-0.5.1-gcc-10.2.1-cvewwn6
module load mochi-margo-0.9.4-gcc-10.2.1-wi5xj2h
module load mochi-thallium-0.7-gcc-10.2.1-mct2gwt
module load hclROCE

# indicate which compiler for C/C++
echo "4. Setting which C/C++ compiler is used..."
export C_COMPILER=mpicc
export CXX_COMPILER=mpicxx

# run cmake
echo "5. Running cmake to config..."
cmake -DHCL_ENABLE_THALLIUM_ROCE=true -DCMAKE_C_COMPILER=${C_COMPILER} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
    -DENABLE_PARALLEL_OMP=1 \
    -DENABLE_GDB_DEBUG=1 \
    ..
