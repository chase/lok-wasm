#include <unx/freetypetextrender.hxx>
#include <SkFontMgr.h>
#include <SkFontMgr_fontconfig.h>

class GenericSalLayout;
class SalGraphics;
typedef struct _skia skia_t;


class VCL_DLLPUBLIC SvpSkiaTextRender final : public FreeTypeTextRenderImpl
{

public:
    virtual void                DrawTextLayout(const GenericSalLayout&, const SalGraphics&) override;
    virtual void                ClearDevFontCache() override;

private:
    static inline sk_sp<SkFontMgr> fontManager;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
