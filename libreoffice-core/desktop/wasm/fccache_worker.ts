import LOK from './soffice';

async function run() {
  const { getDirectoryFiles, Document, createFontConfigCache } = await LOK();
  new Document(`private:factory/swriter`); // force a full load
  createFontConfigCache(); // make the font config cache
  const files = getDirectoryFiles('/instdir/share/fontconfig/cache');
  postMessage({ files });
}

run();
