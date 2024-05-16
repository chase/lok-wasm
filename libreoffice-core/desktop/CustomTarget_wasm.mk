$(eval $(call gb_CustomTarget_CustomTarget,desktop/wasm))

$(eval $(call gb_CustomTarget_register_targets,desktop/wasm,\
	wasm.done \
	package.json \
	version \
	lok_enums.js \
	shared.d.ts \
	tile_renderer_worker.d.ts \
	index.d.ts \
	worker.d.ts \
	fccache.d.ts \
	fccache_worker.d.ts \
	firefox_Atomic_waitAsync_worker.d.ts \
))

$(call gb_CustomTarget_get_workdir,desktop/wasm)/wasm.done : \
	$(SRCDIR)/desktop/wasm/package.json

$(call gb_CustomTarget_get_workdir,desktop/wasm)/lok_enums.ts : $(SRCDIR)/include/LibreOfficeKit/LibreOfficeKitEnums.h
	node $(SRCDIR)/desktop/wasm/generate_ts_enums.js < $< > $@

$(call gb_CustomTarget_get_workdir,desktop/wasm)/lok_enums.js \
$(call gb_CustomTarget_get_workdir,desktop/wasm)/lok_enums.d.ts \
	: $(call gb_CustomTarget_get_workdir,desktop/wasm)/lok_enums.ts
	cd $(SRCDIR)/desktop/wasm/ && \
		([ ! -f node_modules/.bin/tsc ] && npm install || true) && \
		node node_modules/.bin/tsc --skipLibCheck --declaration --module ESNext --target ESNext --moduleResolution bundler --lib ESNext --outDir $(call gb_CustomTarget_get_workdir,desktop/wasm) $<

$(call gb_CustomTarget_get_workdir,desktop/wasm)/package.json : $(SRCDIR)/desktop/wasm/package.json
	sed -e 's|%PRODUCTVERSION%|$(PRODUCTVERSION)|g' \
	    < $< > $@

$(call gb_CustomTarget_get_workdir,desktop/wasm)/version :
	echo 'v$(PRODUCTVERSION)' > $@

$(call gb_CustomTarget_get_workdir,desktop/wasm)/soffice.d.ts : $(SRCDIR)/desktop/wasm/soffice.d.ts
	$(call gb_Helper_copy_if_different_and_touch,$^,$@)

$(call gb_CustomTarget_get_workdir,desktop/wasm)/shared.d.ts : $(SRCDIR)/desktop/wasm/shared.d.ts
	$(call gb_Helper_copy_if_different_and_touch,$^,$@)

$(call gb_CustomTarget_get_workdir,desktop/wasm)/%.d.ts \
$(call gb_CustomTarget_get_workdir,desktop/wasm)/%.js \
	: $(SRCDIR)/desktop/wasm/%.ts \
	| $(INSTDIR)/$(LIBO_BIN_FOLDER)/soffice.mjs
		cd $(SRCDIR)/desktop/wasm/ && \
			([ ! -f node_modules/.bin/tsc ] && npm install || true) && \
			node node_modules/.bin/tsc -p tsconfig.json --outDir $(call gb_CustomTarget_get_workdir,desktop/wasm)
