#pragma once

#include "comphelper/solarmutex.hxx"
#include "sal/types.h"
#include "salinst.hxx"
#include "salusereventlist.hxx"
#include "vcl/dllapi.h"
#include "vcl/svapp.hxx"
class SkiaSalFrame;
class SalFrame;

class SvpSalInstance;

class SalYieldMutex : public comphelper::SolarMutex
{

protected:
    virtual void            doAcquire( sal_uInt32 nLockCount ) override;
    virtual sal_uInt32      doRelease( bool bUnlockAll ) override;

public:
    SalYieldMutex();
    virtual ~SalYieldMutex() override;

    virtual bool IsCurrentThread() const override;
};


class VCL_DLLPUBLIC SkiaSalInstance : public SalInstance, public SalUserEventList
{
    friend class SkiaSalFrame;

    virtual void ProcessEvent( SalUserEvent aEvent ) override;

public:
    virtual void TriggerUserEventProcessing() override;

    static std::list<const ApplicationEvent*> aAppEventList;

    SkiaSalInstance();
    virtual ~SkiaSalInstance() override;

    virtual OpenGLContext*  CreateOpenGLContext() override;
    virtual void            AddToRecentDocumentList(const OUString& rFileUrl, const OUString& rMimeType,
                                                    const OUString& rDocumentService) override;
};
