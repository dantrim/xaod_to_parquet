FROM atlas/analysisbase:21.2.192

USER root

RUN source /release_setup.sh && \
    sudo yum install -y vim && \
    wget https://github.com/apache/arrow/archive/refs/tags/apache-arrow-6.0.0.tar.gz && \
		tar xvf apache-arrow-6.0.0.tar.gz && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DARROW_WITH_ZLIB=ON -DARROW_WITH_SNAPPY=ON -DARROW_PARQUET=ON -DARROW_FILESYSTEM=ON -S ./arrow-apache-arrow-6.0.0/cpp/ -B build && \
    cmake --build build --clean-first --parallel && \
    cd build && \
    make install && \
    cd .. && \
    rm -rf *arrow* *build* && \
    ln -s /usr/lib64/cmake/arrow /usr/lib64/cmake/parquet
