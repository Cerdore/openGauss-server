# contrib/external_gpu_join/Makefile

MODULE_big = external_gpu_join
OBJS = external_join.o nestLoopJoin.o kernel.o
PGFILEDESC = "external_gpu_join - working"

#$(info    PGXS VAR is $(MODULE_big))


ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
#$(info    step into USE_PGXS)
include $(PGXS)
else 
$(info    enter else)
subdir = contrib/external_gpu_join
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

# CC = g++  -I ../../src/gausskernel -I /usr/local/cuda/include

CXX = g++ -L/usr/local/cuda/lib64 -lcutil -lcudpp -lcuda -lcudart -I/usr/local/cuda/include -I ../../src/gausskernel

CC += -L/usr/local/cuda/lib64 -lcuda -lcudart
# CXX = g++ -I /usr/local/cuda/include
# CXXFLAGS = -I /usr/local/cuda/include # -fPIC -std=c++11 -Wno-deprecated-register -Wno-deprecated-declarations -Wall
# $(info    line27: CXXFLAGS is $(CXXFLAGS))