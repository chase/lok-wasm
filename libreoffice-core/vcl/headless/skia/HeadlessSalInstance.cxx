#include <headless/skia/HeadlessSalInstance.hxx>
#include "headless/svpdata.hxx"
#include "headless/svpdummies.hxx"
#include <vcl/layout.hxx>
#include <vcl/settings.hxx>

HeadlessSkiaSalInstance::HeadlessSkiaSalInstance( std::unique_ptr<SalYieldMutex> pMutex )
    : SvpSalInstance( std::move(pMutex) )
{
}

HeadlessSkiaSalInstance::~HeadlessSkiaSalInstance()
{
}

HeadlessSkiaSalInstance *HeadlessSkiaSalInstance::getInstance()
{
    if (!ImplGetSVData())
        return NULL;
    return static_cast<HeadlessSkiaSalInstance *>(GetSalInstance());
}

extern "C" SalInstance *create_SalInstance()
{
    HeadlessSkiaSalInstance* pInstance = new HeadlessSkiaSalInstance( std::make_unique<SvpSalYieldMutex>() );
    new SvpSalData();
    return pInstance;
}
