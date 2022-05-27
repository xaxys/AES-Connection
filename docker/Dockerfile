FROM gcc:latest

ENV TZ=Asia/Shanghai

RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && \
    echo $TZ > /etc/timezone

RUN dpkg-divert --remove /usr/bin/gfortran && \
    dpkg-divert --divert /usr/bin/gfortran.orig --rename /usr/local/bin/gfortran

RUN sed -i "s@/deb.debian.org/@/mirrors.aliyun.com/@g" /etc/apt/sources.list && \
    sed -i "s@/security.debian.org/@/mirrors.aliyun.com/@g" /etc/apt/sources.list && \
    apt-get update && apt-get -y full-upgrade --fix-missing && \
    apt-get install -y build-essential libgtest-dev libbenchmark-dev cmake gdb binutils clang-format libboost-all-dev

RUN cd /usr/src/gtest && \
    cmake CMakeLists.txt && \
    make && \
    cp lib/*.a /usr/lib

WORKDIR /app

ENTRYPOINT bash
