#! /usr/bin/env bash

numactl -N 0 -m 0 ./perma-bench --dram --no-numa &&
numactl -N 0 -m 1 ./perma-bench --dram --no-numa &&
numactl -N 0 -m 2 ./perma-bench --dram --no-numa
