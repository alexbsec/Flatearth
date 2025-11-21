#!/bin/bash

set echo on

mkdir -p bin

pushd engine
./run.sh
popd

pushd testbed
./run.sh
popd
