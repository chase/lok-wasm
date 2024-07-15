# Macro LOK

This is a private project that forks LibreOffice to provide functionality and fixes that don't fit in [the upstream project](https://github.com/LibreOffice/core).

This upstream changes are open sourced under the [Mozilla Public License 2.0](LICENSE), newly added files are considered proprietary.

This project is not a part of the official LibreOffice project, nor endorsed by the Document Foundation.

# Prerequisites

You must use Linux on x64, otherwise the build will not work.

On Ubuntu/Debian/Pop_OS!, you should install these packages:

```bash
apt-get install -y --no-install-recommends \
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
  sudo \
  locales \
  libnss3
```

Setup the repo:

```bash
git clone https://github.com/coparse-inc/lok-wasm
./scripts/setup
```

# Building

```bash
# Run configure for the initial build or any configuration changes
./scripts/configure dev
# if you're using VS Code or clangd in vim, run this
./scripts/clangd
# Run build for any code changes
./scripts/build
```

# Debugging

Make a debug build:

```bash
# Clean the existing build
(cd libreoffice-core/ && make clean)
# Run configure for debug
./scripts/configure debug
# Run build for any code changes
./scripts/build
```

Use Chrome with [the C/C++ WASM debugging tools extension](https://goo.gle/wasm-debugging-extension) installed.

Use Ctrl/Cmd+P in the Dev Tools to quickly navigate to the `.cxx` file you need to debug.

You can add breakpoints as necessary in the Dev Tools, but conditional breakpoints still aren't supported.  
If you need to add a conditional breakpoint, use an if statement with a log to set a break point on inside of your code:  
```
if (myCondition == true) {
    SAL_WARN("debug", "here");
}
```


Expand the LOK logs to see the full stack trace, clicking on the `.cxx` file will jump to the source for the file.

# QA Env

Make a build first, then:

```bash
# change to the qa-env directory
cd qa-env
# install the qa-env dependencies
npm install
# run the qa-env
npm run dev
```

# Docs

<!-- TIP: in neovim, you can use `gf` to go to the file linked if the cursor is between ( ) -->

- [Remote Development](./remote_dev.md)
- [Important Files](./important_files.md)
