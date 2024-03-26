import { defineConfig } from "vite";
import path from "node:path";
import { fileURLToPath } from "url";
import { dirname } from "path";
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

export default defineConfig({
  logLevel: "silent",
  server: {
    fs: {
      allow: [
        path.resolve(__dirname, "src"),
        path.resolve(__dirname, "../libreoffice-core/instdir/program"),
      ],
    },
  },
  resolve: {
    alias: {
      "@lok": path.resolve(__dirname, "../libreoffice-core/instdir/program"),
    },
  },
  plugins: [
    {
      name: "configure-response-headers",
      configureServer: (server) => {
        server.middlewares.use((_req: any, res, next) => {
          res.setHeader("Cross-Origin-Embedder-Policy", "require-corp");
          res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
          if (_req.headers.host.includes("localhost")) {
            res.setHeader(
              "Access-Control-Allow-Origin",
              `http://${_req.headers.host}`
            );
          }
          next();
        });
      },
    },
  ],
});
