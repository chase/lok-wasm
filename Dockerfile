FROM emscripten/emsdk:3.1.73-arm64

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
  g++-12

COPY emsdk-patches/ /emsdk-patches

RUN cd /emsdk && for patch_file in /emsdk-patches/*.patch; do patch -p1 -i $patch_file; done
