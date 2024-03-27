
#include "salgdi.hxx"
#include "salinst.hxx"
#include "salvd.hxx"
#include "vcl/dllapi.h"
#include "vcl/salgtype.hxx"
#include "vcl/sysdata.hxx"
#include <memory>
class VCL_PLUGIN_PUBLIC SkiaSalInstance : public SalInstance
{
public:
    virtual std::unique_ptr<SalVirtualDevice> createVirtualDevice(SalGraphics& rGraphics, tools::Long width, tools::Long height, DeviceFormat pFormat, SystemGraphicsData const* pData);

};
