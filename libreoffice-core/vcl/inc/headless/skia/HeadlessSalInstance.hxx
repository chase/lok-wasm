#include "vcl/dllapi.h"
#include <headless/svpinst.hxx>
#include <headless/svpframe.hxx>


class VCL_DLLPUBLIC HeadlessSkiaSalInstance final : public SvpSalInstance
{
public:
    HeadlessSkiaSalInstance(std::unique_ptr<SalYieldMutex> pMutex);
    virtual ~HeadlessSkiaSalInstance();
    static HeadlessSkiaSalInstance* getInstance();

    SalSystem* CreateSalSystem() override;

    css::uno::Reference<css::uno::XInterface>
    CreateClipboard(const css::uno::Sequence<css::uno::Any>& i_rArguments) override;

    void GetWorkArea(tools::Rectangle& rRect);
    SalFrame* CreateFrame(SalFrame* pParent, SalFrameStyleFlags nStyle) override;
    SalFrame* CreateChildFrame(SystemParentData* pParent, SalFrameStyleFlags nStyle) override;
};

class SalData
{
public:
    static void ensureThreadAutoreleasePool(){};

    explicit SalData();
    virtual ~SalData();
};
