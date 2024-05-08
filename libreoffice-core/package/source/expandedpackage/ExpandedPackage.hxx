#ifndef INCLUDED_PACKAGE_INC_EXPANDEDPACKAGE_HXX
#define INCLUDED_PACKAGE_INC_EXPANDEDPACKAGE_HXX

#include "com/sun/star/uno/XComponentContext.hdl"
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/xml/crypto/CipherID.hpp>
#include <comphelper/refcountedmutex.hxx>
#include <rtl/ref.hxx>

#include <vector>
#include <optional>

struct PackageFile
{
    OUString path;
    css::uno::Sequence<sal_Int8> content;

    PackageFile(OUString path, css::uno::Sequence<sal_Int8> content)
        : path(std::move(path))
        , content(std::move(content))
    {
    }
};

class ExpandedPackage final : public cppu::WeakImplHelper
                    <
                       css::lang::XInitialization,
                       css::lang::XSingleServiceFactory,
                       css::lang::XServiceInfo,
                       css::container::XHierarchicalNameAccess,
                       css::util::XChangesBatch,
                       css::beans::XPropertySet
                    >
{
    rtl::Reference<comphelper::RefCountedMutex> m_aMutexHolder;
    std::unordered_map<OUString, PackageFile> m_aPackageFiles;
    sal_Int32 m_nFormat;
    const css::uno::Reference<css::uno::XComponentContext> m_xContext;

public:
    ExpandedPackage(css::uno::Reference<css::uno::XComponentContext> xContext);
    virtual ~ExpandedPackage() override;

    // XInitialization
    virtual void SAL_CALL initialize(const css::uno::Sequence<css::uno::Any>& aArguments) override;
    // XHierarchicalNameAccess
    virtual css::uno::Any SAL_CALL getByHierarchicalName(const OUString& aName) override;
    virtual sal_Bool SAL_CALL hasByHierarchicalName(const OUString& aName) override;
    // XSingleServiceFactory
    virtual css::uno::Reference<css::uno::XInterface> SAL_CALL createInstance() override;
    virtual css::uno::Reference<css::uno::XInterface> SAL_CALL createInstanceWithArguments(const css::uno::Sequence<css::uno::Any>& aArguments) override;
    // XChangesBatch
    virtual void SAL_CALL commitChanges() override;
    virtual sal_Bool SAL_CALL hasPendingChanges() override;
    virtual css::uno::Sequence<css::util::ElementChange> SAL_CALL getPendingChanges() override;
    // XPropertySet
    virtual css::uno::Reference<css::beans::XPropertySetInfo> SAL_CALL getPropertySetInfo() override;
    virtual void SAL_CALL setPropertyValue(const OUString& aPropertyName, const css::uno::Any& aValue) override;
    virtual css::uno::Any SAL_CALL getPropertyValue(const OUString& PropertyName) override;
    virtual void SAL_CALL addPropertyChangeListener(const OUString& aPropertyName, const css::uno::Reference<css::beans::XPropertyChangeListener>& xListener) override;
    virtual void SAL_CALL removePropertyChangeListener(const OUString& aPropertyName, const css::uno::Reference<css::beans::XPropertyChangeListener>& aListener) override;
    virtual void SAL_CALL addVetoableChangeListener(const OUString& PropertyName, const css::uno::Reference<css::beans::XVetoableChangeListener>& aListener) override;
    virtual void SAL_CALL removeVetoableChangeListener(const OUString& PropertyName, const css::uno::Reference<css::beans::XVetoableChangeListener>& aListener) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;

    // Additional methods
    css::uno::Sequence<sal_Int8> readFile(const OUString& path);
};

#endif
