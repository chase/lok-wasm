#include <salgdi.hxx>
#include <skia/gdiimpl.hxx>

class VCL_PLUGIN_PUBLIC SkiaSalGraphicsBackend final : public SkiaSalGraphicsImpl
{

public:
    SkiaSalGraphicsBackend(SkiaSalGraphics& rParnet);

};
