$(eval $(call gb_Package_Package,desktop_wasm,$(call gb_CustomTarget_get_workdir,desktop/wasm)))

$(eval $(call gb_Package_set_outdir,desktop_wasm,$(INSTROOT)))

$(eval $(call gb_Package_add_files,desktop_wasm,$(LIBO_BIN_FOLDER),\
	package.json \
	version \
	index.js \
	index.d.ts \
	worker.js \
	worker.d.ts \
	shared.js \
	shared.d.ts \
	tile_renderer_worker.js \
	tile_renderer_worker.d.ts \
	lok_enums.js \
	lok_enums.d.ts \
	soffice.d.ts \
	fccache_worker.js \
	fccache_worker.d.ts \
	fccache.js \
	fccache.d.ts \
	firefox_Atomic_waitAsync_worker.js \
	firefox_Atomic_waitAsync_worker.d.ts \
))
