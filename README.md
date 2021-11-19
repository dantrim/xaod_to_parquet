# xaod_to_parquet
An area to test reading in ATLAS xAOD format and writing out to Parquet

## Getting the Code

Clone the repository with the `--recursive` flag to pick up [parquet-writer](https://github.com/dantrim/parquet-writer):

```verbatim
git clone --recursive https://github.com/dantrim/xaod_to_parquet.git
```

## Building

We must have an environment with both ATLAS AnalysisBase and Apache Arrow & Parquet libraries.
The image `dantrim/analysisbase-pq` is built and tagged for several AnalysisBase releases.
It simply adds the Apache Arrow & Parquet libraries on top of the already existing
[ATLAS AnalysisBase images](https://hub.docker.com/r/atlas/analysisbase). For example:

```verbatim
docker pull dantrim/analysisbase-pq:21.2.192
```

Once you have pulled the image:

```verbatim
cd /path/to/xaod_to_parquet
docker run --rm -ti -v ${PWD}:/workdir dantrim/analysisbase-pq:21.2.192 /bin/bash
source /release_setup.sh
cd src/
mkdir build
cd build
cmake -DCMAKE_MODULE_PATH=$(find /usr/lib64 -type d -name arrow) ..
make
source x86*/setup.sh
```

## Toying Around

Currently a dummy executable [dump-parquet](src/xaod_to_parquet/util/dump-parquet.cpp) gets built, which 
simply writes a Parquet file provided the test layout file [test_layout.json](src/xaod_to_parquet/layouts/test_layout.json).

After successfully [building](#building) you can do:

```verbatim
cd /path/to/build
./x86_64-centos7-gcc8-opt/bin/dump-parquet --layout ../xaod_to_parquet/layouts/test_layout.json
```

After which you will have a Parquet file `my_dataset.parquet`:

```verbatim
parquet-tools show my_dataset.parquet
+-------+-------+------------------+
|   foo |   bar | baz              |
|-------+-------+------------------|
|    42 |    42 | [42.  42.1 42.2] |
|    42 |    42 | [42.  42.1 42.2] |
|    42 |    42 | [42.  42.1 42.2] |
|    42 |    42 | [42.  42.1 42.2] |
|    42 |    42 | [42.  42.1 42.2] |
|    42 |    42 | [42.  42.1 42.2] |
|    42 |    42 | [42.  42.1 42.2] |
|    42 |    42 | [42.  42.1 42.2] |
|    42 |    42 | [42.  42.1 42.2] |
|    42 |    42 | [42.  42.1 42.2] |
+-------+-------+------------------+
```

