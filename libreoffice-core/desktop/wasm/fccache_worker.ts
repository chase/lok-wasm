import LOK from './soffice';

async function run() {
  const { getDirectoryFiles, Document } = await LOK();
  new Document(`private:factory/swriter`); // force a full load
  const files = getDirectoryFiles('/instdir/share/fontconfig/cache');
  postMessage({ files });
}

run();
