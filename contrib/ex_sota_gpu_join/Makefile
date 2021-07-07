# contrib/external_gpu_join/Makefile

MODULE_big = ex_sota_gpu_join
OBJS = external_join.o nestLoopJoin.o   kernel.o linearprobing.o bridge_link.o bridge.o partition-primitives_link.o partition-primitives.o join-primitives_link.o join-primitives.o hash_join_clustered_probe_link.o hash_join_clustered_probe.o generator_ETHZ_link.o generator_ETHZ.o common_link.o common.o common-host_link.o common-host.o
PGFILEDESC = "ex_sota_gpu_join - working"

#$(info    PGXS VAR is $(MODULE_big))


ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
#$(info    step into USE_PGXS)
include $(PGXS)
else 
$(info    enter else)
subdir = contrib/ex_sota_gpu_join
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

# CC = g++  -I ../../src/gausskernel -I /usr/local/cuda/include

CXX = g++ -L/usr/local/cuda/lib64 -lcutil -lcudpp -lcuda -lcudart -lcudadevrt -fopenmp -I/usr/local/cuda/include -I ../../src/gausskernel -fPIC
#CXX = nvcc --compiler-options "-L/usr/local/cuda/lib64 -lcutil -lcudpp -lcuda -lcudart -I/usr/local/cuda/include -I ../../src/gausskernel"

CC += -L/usr/local/cuda/lib64 -lc -lcuda -lcudart -lcudadevrt -fopenmp -I/usr/local/cuda/include -lgomp -lnuma
# CXX = g++ -I /usr/local/cuda/include
# CXXFLAGS = -I /usr/local/cuda/include # -fPIC -std=c++11 -Wno-deprecated-register -Wno-deprecated-declarations -Wall
# $(info    line27: CXXFLAGS is $(CXXFLAGS))