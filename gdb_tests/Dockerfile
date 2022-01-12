FROM debian:buster as build
ENV DEBIAN_FRONTEND=noninteractive
# install stuff for building
RUN apt-get update && apt-get install -y --no-install-recommends build-essential texinfo git fakeroot python3 python3-dev python3-pip liblzma-dev lzma libgmp-dev file libtinfo-dev libncurses-dev bison flex
WORKDIR /build/
# clone
RUN git clone --depth 1 git://sourceware.org/git/binutils-gdb.git
WORKDIR /build/binutils-gdb
# create dirs
RUN mkdir -p custom-gdb-amd64/usr/ && mkdir -p custom-gdb-amd64/DEBIAN
# configure
RUN ./configure --enable-targets=all --prefix=$PWD/custom-gdb-amd64 --enable-debug --enable-gdbserver --disable-dependency-tracking --with-lzma --with-python=/usr/bin/python3
# build
RUN make && make install
# create debfile
COPY control ./custom-gdb-amd64/DEBIAN/
RUN fakeroot dpkg-deb --build custom-gdb-amd64

FROM debian:buster
ENV DEBIAN_FRONTEND=noninteractive
# install python for gdb
RUN apt-get update && apt-get install -y --no-install-recommends python3 python3-dev python3-pip
COPY --from=build /build/binutils-gdb/custom-gdb-amd64.deb /tmp/
RUN dpkg -i /tmp/custom-gdb-amd64.deb && rm -rf /tmp/custom-gdb-amd64.deb