import { defineConfig } from "vite";
import solid from "vite-plugin-solid";
import tailwindcss from '@tailwindcss/vite'
// @ts-ignore
import path from "path";
declare const __dirname: string;

export default defineConfig({
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
    tailwindcss(),
    solid({exclude: [/libreoffice-core/]}),
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
