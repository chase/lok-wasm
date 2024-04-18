# Remote Development Environment

A lot of our engineers work on macOS on ARM-based machines. Due LibreOffice having problems with cross-compiling across architectures, we have a containerized remote development environment for LOK WASM ready to go.

## Mutagen.io

Mutagen is an excellent tool that allows transparent, fast syncing of local project files with the remote development environment. This allows you to work on the project locally, without any latency to your keypresses. It also allows easy, secure forwarding of any ports from the remote development environment.

### Installing Mutagen with `brew`

```bash
brew install mutagen-io/mutagen/mutagen
```

### Using Mutagen

Start syncing and forwarding:

```bash
# This can take a while at the beginning, changes after that are pretty fast
mutagen project start
```

Stop syncing and forwarding:

```bash
mutagen project terminate
```

Check sync status:

```bash
mutagen sync monitor
```

## Remote Scripts

You'll still need to trigger builds, among other things.

SSH into your development environment:

```bash
./remote/ssh
```

Make a build:

```bash
./remote/build
```

Start the QA environment remotely:

```bash
# this is accessible from your localhost, thanks to mutagen's connection forwarding
# it's slower to load because it's not a local server, so you can also wait for a build to finish and run the QA env locally
./remote/qa
```

### Typical Development Cycle

Initial setup:

```bash
# sync project files
mutagen project start
# setup emscripten toolchain for clangd
./scripts/setup.sh
# build the compile commands for clangd and transform them for local use
./remote/clangd
```

After making modifications, make a build (it will automatically sync locally, but takes about as long as downloading 100MiB+ files in addition to the build time):

```bash
# make a build
./remote/build
# optional: wait for file sync activity to say Awaiting changes, that means everything is synced
mutagen sync monitor
```
