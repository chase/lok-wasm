/// <reference types="emscripten" />
let mountBlobCreated = false;
const BLOB_DIR = '/blob';
Module['mountBlob'] = function (name, blob) {
  if (!mountBlobCreated) {
    FS.mkdir(BLOB_DIR);
    mountBlobCreated = true;
  }

  FS.mount(
    WORKERFS,
    {
      blobs: [{ name: name, data: blob }],
    },
    BLOB_DIR
  );

  return `${BLOB_DIR}/${name}`;
};

Module['unmountBlob'] = function () {
  FS.unmount(BLOB_DIR);
};

Module['readUnlink'] = function (path) {
  const file = FS.readFile(path);
  FS.unlink(path);
  return file;
};

if (Module.callbackHandlers == null) {
  Module['callbackHandlers'] = {
    renderCallback(ref, type, payload) {
      console.error('renderCallback fired but not handled', ref, type, payload);
    },
    callback(ref, type, payload) {
      console.error('callback fired but not handled', ref, type, payload);
    },
  };
}

Module['getDirectoryFiles'] = function (dir) {
  const result = [];
  FS.readdir(dir).forEach((file) => {
    if (file !== '.' && file !== '..') {
      const contents = FS.readFile(`${dir}/${file}`);
      result.push({ name: file, buffer: contents.buffer });
    }
  });
  return result;
};
