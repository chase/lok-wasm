import { chromium, devices } from "playwright";
import { stdout } from "node:process";
import { createServer } from "vite";
import { fileURLToPath } from "url";
import { dirname, join } from "path";
import http from "http";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const PORT = 18777;

function waitForServerResponse(url, maxAttempts = 10, interval = 2000) {
  return new Promise((resolve, reject) => {
    let attempts = 0;

    const checkServer = () => {
      http
        .get(url, (res) => {
          if (res.statusCode === 200) {
            resolve();
          } else {
            console.log(res.statusCode);
            retry();
          }
        })
        .on("error", (err) => {
          console.log(err);
          retry();
        });
    };

    const retry = () => {
      if (++attempts < maxAttempts) {
        setTimeout(checkServer, interval);
      } else {
        reject(new Error("Max attempts reached without success"));
      }
    };

    checkServer();
  });
}

(async () => {
  const server = await createServer({
    configFile: join(__dirname, "fccache_vite.config.ts"),
    server: {
      port: PORT,
    },
  });
  await server.listen();
  const { port } = server.httpServer.address();
  const url = `http://localhost:${port}/index_fccache.html`;
  await waitForServerResponse(url);
  const browser = await chromium.launch();
  const context = await browser.newContext(devices["Desktop Chrome"]);
  const page = await context.newPage();
  await page.goto(url);

  let saved = 0;
  page.on("download", async (download) => {
    const filename = download.suggestedFilename();
    const path = `${__dirname}/workdir/${filename}`;
    stdout.write(`${path}@/instdir/share/fontconfig/cache/${filename} `);
    await download.saveAs(path);
    saved++;
  });

  const count = await page.evaluate(async () => {
    return window.run();
  });
  await new Promise((resolve) => {
    const int = setInterval(() => {
      if (saved === count) {
        clearInterval(int);
        resolve();
      }
    }, 10);
  });

  await context.close();
  await browser.close();
  await server.close();
})();
