###
 # @Author: your name
 # @Date: 2021-07-08 01:52:58
 # @LastEditTime: 2021-07-08 01:53:33
 # @LastEditors: Please set LastEditors
 # @Description: In User Settings Edit
 # @FilePath: /openGauss-server/contrib/external_gpu_join/build.sh
### 
nvcc --compiler-options "-Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC -fopenmp -Ofast -DGPU -DCUDNN" -c linearprobing.cu

nvcc --compiler-options "-Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC -fopenmp -Ofast -DGPU -DCUDNN" -c kernel.cu

make install