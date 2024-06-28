# Macro LOK

This is a private project that forks LibreOffice to provide functionality and fixes that don't fit in [the upstream project](https://github.com/LibreOffice/core).

This upstream changes are open sourced under the [Mozilla Public License 2.0](LICENSE), newly added files are considered proprietary.

This project is not a part of the official LibreOffice project, nor endorsed by the Document Foundation.

# Prerequisites

TODO: List pre-reqs for each platform

Setup the repo:
```bash
git clone https://github.com/coparse-inc/lok-wasm
./scripts/setup
```

# Building

```bash
# Run configure.sh for the initial build or any configuration changes
./scripts/configure
# if you're using VS Code or clangd in vim, run this
./scripts/clangd
# Run build.sh for any code changes
./scripts/build
```

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
