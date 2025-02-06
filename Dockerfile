ARG ARCH=
ARG VER=3.1.73
FROM docker.io/emscripten/emsdk:${VER}${ARCH}

RUN apt update && apt-get install -y --no-install-recommends \
  git \
  build-essential \
  zip \
  nasm \
  python3 \
  python3-dev \
  autoconf \
  gperf \
  xsltproc \
  libxml2-utils \
  bison \
  flex \
  pkg-config \
  ccache \
  openssh-server \
  cmake \
  locales \
  libnss3 \
  automake \
  g++-12 \
  libpng-dev

COPY emsdk-patches/ /emsdk-patches

RUN cd /emsdk && for patch_file in /emsdk-patches/*.patch; do patch -p1 -i $patch_file; done
