#include "SkSurface.h"
#include <salvd.hxx>
#include <vcl/salgtype.hxx>
#include <basegfx/vector/b2ivector.hxx>

#include <vector>

class SvpSalGraphics;

class VCL_DLLPUBLIC SvpSalVirtualDevice : public SalVirtualDevice
{
    SkSurface*                          m_pRefSurface;
    SkSurface*                          m_pSurface;
    bool                                m_bOwnsSurface; // nearly always true, except for edge case of tdf#127529
    basegfx::B2IVector                  m_aFrameSize;
    std::vector< SvpSalGraphics* >      m_aGraphics;

    bool CreateSurface(tools::Long nNewDX, tools::Long nNewDY, sal_uInt8 *const pBuffer);

protected:
    SvpSalGraphics* AddGraphics(SvpSalGraphics* aGraphics);

public:
    SvpSalVirtualDevice(SkSurface* pRefSurface, SkSurface* pPreExistingTarget);
    virtual ~SvpSalVirtualDevice() override;

    // SalVirtualDevice
    virtual SalGraphics*    AcquireGraphics() override;
    virtual void            ReleaseGraphics( SalGraphics* pGraphics ) override;

    virtual bool        SetSize( tools::Long nNewDX, tools::Long nNewDY ) override;
    virtual bool        SetSizeUsingBuffer( tools::Long nNewDX, tools::Long nNewDY,
                                            sal_uInt8 * pBuffer
                                          ) override;

    SkSurface* GetSurface() const { return m_pSurface; }

    // SalGeometryProvider
    virtual tools::Long GetWidth() const override;
    virtual tools::Long GetHeight() const override;
};
