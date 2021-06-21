module use ~/localLibs/myModules
module use ~/spack/share/spack/lmod/linux-sles15-zen2
module load mpich-3.3.2-gcc-10.2.1-ooqbcd3
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
export OMP_PROC_BIND=true
export OMP_PLACES=cores
unset KMP_AFFINITY
export I_MPI_PIN_DOMAIN=auto
export I_MPI_PIN=1
