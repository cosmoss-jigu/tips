# TIPS: Making Volatile Indexes Persistent With DRAM-NVMM Tiering (ATC 2021, NVMW 2021)

This repository contains the source code for TIPS library and seven volatile
indexes that are converted using TIPS. All the TIPS-indexes are integrated with
YCSB and the performance results presented in the ATC 2021 paper can be
reproduced from this repository. Please find the link to the paper, ATC video
presentation, and NVMW video presentations below.

- [ATC-2021-paper](https://www.usenix.org/conference/atc21/presentation/krishnan)
- [ATC-2021-talk](https://www.youtube.com/watch?v=v80-xvvy3Ms)
- [NVMW-2021-long-video](https://www.youtube.com/watch?v=4rSTHLAvq7k)
- [NVMW-2021-short-video](https://www.youtube.com/watch?v=arRwT5W9zSU)

## Running TIPS-enabled Indexes 

### Preparing the YCSB workload files
- Clone TIPS repo
```
git clone git@github.com:cosmoss-vt/tips.git
cd tips/index-microbench
```
- Download and setup YCSB 
```
curl -O --location https://github.com/brianfrankcooper/YCSB/releases/download/0.11.0/ycsb-0.11.0.tar.gz
tar xfvz ycsb-0.11.0.tar.gz
mv ycsb-0.11.0 YCSB
mkdir workloads
sh generate_all_workloads.sh
```
### Compiling TIPS source and benchmarks
- Compile the TIPS source
```
cd tips/tips-lib/src
make 
```
- Prepare the build files for benchmarks
```
cd ../../
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cp ycsb ../
```
### Running the bechmark
```
sh rm.sh
sudo ./ycsb tips <index-- hlist/lfhlist/bst/lfbst/art/btree/clht> 
<workload type-- a/b/c/d/e> randint <uniform/zipfian> <number of threads>
```

### Note 
- The default workload spec is set at 32M keys and uniform distribution; please
  modify the tips-ycsb/index-microbench/workload_spec file to change configs.
- TIPS uses PMDK failure-atomic memory allocator, for a fair comparison against
  TIPS porting your application to use PMDK is recommended.  
- The default PMEM path is configured as /mnt/pmem0/, please change tips-ycsb/rm.sh and tips-ycsb/include/pmdk.h file
  if you use a different path.

## Contact 
Reach out to us at madhavakrishnan@vt.edu and changwoo@vt.edu

## Citation
```
@inproceedings {krishnan:tips,
title = {{TIPS} : Making Volatile Index Structures Persistent with DRAM-NVMM Tiering},
booktitle = {2021 {USENIX} Annual Technical Conference ({USENIX} {ATC} 21)},
year = {2021},
url = {https://www.usenix.org/conference/atc21/presentation/krishnan},
publisher = {{USENIX} Association},
month = jul,
}
```
