# Macro LOK

This is a private project that forks LibreOffice to provide functionality and fixes that don't fit in [the upstream project](https://github.com/LibreOffice/core).

This upstream changes are open sourced under the [Mozilla Public License 2.0](LICENSE), newly added files are considered proprietary.

This project is not a part of the official LibreOffice project, nor endorsed by the Document Foundation.

# Prerequisites

TODO: List pre-reqs for each platform

Setup the repo:
```bash
git clone https://github.com/coparse-inc/lok-wasm
./scripts/setup.sh
# if you're using VS Code or clangd in vim, run this
./scripts/prepare-clangd.sh
```

# Building

```bash
# Run configure.sh for the initial build or any configuration changes
./scripts/configure.sh
# Run build.sh for any code changes
./scripts/build.sh
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

- [Important Files](./important_files.md)
