# 先执行这个，再执行下边的link 

nvcc --compiler-options "-Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC -fopenmp -Ofast -DGPU -DCUDNN" -c linearprobing.cu

nvcc --compiler-options "-Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC -fopenmp -Ofast -DGPU -DCUDNN" -c kernel.cu


# debug
# nvcc  -O0 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC -Ofast -DGPU -DCUDNN' -I. -Icub -g -G  -c common-host.cpp 

# nvcc  -O0 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC -Ofast -DGPU -DCUDNN' -I. -Icub -g -G  -c common.cu  

# nvcc  -O0 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC -Ofast -DGPU -DCUDNN' -I. -Icub -g -G  -c generator_ETHZ.cu  

# nvcc  -O0 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC -Ofast -DGPU -DCUDNN' -I.  -I../../src/include -Icub -g -G  -c hash_join_clustered_probe.cu  

# nvcc  -O0 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC -Ofast -DGPU -DCUDNN' -I. -Icub -g -G  -c join-primitives.cu  

# nvcc  -O0 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC  -Ofast -DGPU -DCUDNN' -I. -Icub -g -G  -c partition-primitives.cu  

# nvcc  -O0 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC -Ofast -DGPU -DCUDNN' -I. -Icub -g -G  -c bridge.cu  


# debug link
# nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -o common-host_link.o common-host.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt

# nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -o common_link.o common.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt

# nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -o generator_ETHZ_link.o generator_ETHZ.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt

# nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -I../../src/include -o hash_join_clustered_probe_link.o hash_join_clustered_probe.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt 

# nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -o join-primitives_link.o join-primitives.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt

# nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -o partition-primitives_link.o partition-primitives.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt

# nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -o bridge_link.o bridge.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt


# release
nvcc  -O3 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -fPIC' -I. -Icub   -c  common-host.cpp -o obj/release/common-host.o
nvcc  -O3 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -fPIC' -I. -Icub   -c common.cu  -o obj/release/common.o
nvcc  -O3 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -fPIC' -I. -Icub   -c generator_ETHZ.cu  -o obj/release/generator_ETHZ.o

nvcc  -O3 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -fPIC' -I. -Icub   -c hash_join_clustered_probe.cu  -o obj/release/hash_join_clustered_probe.o
nvcc  -O3 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -fPIC' -I. -Icub   -c join-primitives.cu  -o obj/release/join-primitives.o
nvcc  -O3 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -fPIC' -I. -Icub   -c bridge.cu  -o obj/release/bridge.o
nvcc  -O3 -arch=sm_61 -lineinfo --std=c++11  -lineinfo -rdc=true --default-stream per-thread --expt-relaxed-constexpr --compiler-options='-O3 -fopenmp -mavx2 -mbmi2 -fPIC' -I. -Icub   -c partition-primitives.cu  -o obj/release/partition-primitives.o


# release link
nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -o obj/release/common-host_link.o obj/release/common-host.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt

nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -o obj/release/common_link.o obj/release/common.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt

nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -o obj/release/generator_ETHZ_link.o obj/release/generator_ETHZ.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt

nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -I../../src/include -o obj/release/hash_join_clustered_probe_link.o obj/release/hash_join_clustered_probe.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt 

nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -o obj/release/join-primitives_link.o obj/release/join-primitives.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt

nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -o obj/release/partition-primitives_link.o obj/release/partition-primitives.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt

nvcc -arch=sm_61 -Xcompiler -fPIC -dlink -o obj/release/bridge_link.o obj/release/bridge.o -L/usr/local/cuda/lib64 -lcudart -lcudadevrt

make install
