#include <vcl/dllapi.h>
#include <headless/svpinst.hxx>
#include <tools/link.hxx>
#include <headless/svpframe.hxx>


class SystemFontList;

class VCL_DLLPUBLIC HeadlessSkiaSalInstance : public SvpSalInstance
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
