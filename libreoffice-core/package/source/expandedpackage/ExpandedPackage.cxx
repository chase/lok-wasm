#include "ExpandedPackage.hxx"
#include "com/sun/star/beans/NamedValue.hdl"
#include <com/sun/star/io/NotConnectedException.hpp>
#include <com/sun/star/uno/Exception.hpp>
#include <cppuhelper/supportsservice.hxx>
#include <algorithm>

using namespace com::sun::star;

ExpandedPackage::ExpandedPackage(uno::Reference<uno::XComponentContext> xContext)
    : m_aMutexHolder(new comphelper::RefCountedMutex)
    , m_nFormat(0)
    , m_xContext(std::move(xContext))
{
}

ExpandedPackage::~ExpandedPackage() = default;

void SAL_CALL ExpandedPackage::initialize(const uno::Sequence<uno::Any>& aArguments)
{
}

uno::Any SAL_CALL ExpandedPackage::getByHierarchicalName(const OUString& aName)
{
    ::osl::MutexGuard aGuard(m_aMutexHolder->GetMutex());

    for (const auto& rFile : m_aPackageFiles)
    {
        if (rFile.path == aName)
            return uno::Any(rFile.content);
    }

    throw container::NoSuchElementException(aName, static_cast<cppu::OWeakObject*>(this));
}

sal_Bool SAL_CALL ExpandedPackage::hasByHierarchicalName(const OUString& aName)
{
    ::osl::MutexGuard aGuard(m_aMutexHolder->GetMutex());

    return std::any_of(m_aPackageFiles.begin(), m_aPackageFiles.end(),
        [&aName](const PackageFile& rFile) { return rFile.path == aName; });
}

void SAL_CALL ExpandedPackage::commitChanges()
{}

sal_Bool SAL_CALL ExpandedPackage::hasPendingChanges()
{
    return false;
}

uno::Sequence<util::ElementChange> SAL_CALL ExpandedPackage::getPendingChanges()
{
    return uno::Sequence<util::ElementChange>();
}

uno::Reference<beans::XPropertySetInfo> SAL_CALL ExpandedPackage::getPropertySetInfo()
{
    return nullptr;
}

void SAL_CALL ExpandedPackage::setPropertyValue(const OUString& /*aPropertyName*/, const uno::Any& /*aValue*/)
{
    // TODO @synoet implement if needed
}

uno::Any SAL_CALL ExpandedPackage::getPropertyValue(const OUString& /*PropertyName*/)
{
    return uno::Any();
}

void SAL_CALL ExpandedPackage::addPropertyChangeListener(const OUString& /*aPropertyName*/, const uno::Reference<beans::XPropertyChangeListener>& /*xListener*/)
{
    // TODO @synoet implement if needed
}

void SAL_CALL ExpandedPackage::removePropertyChangeListener(const OUString& /*aPropertyName*/, const uno::Reference<beans::XPropertyChangeListener>& /*aListener*/)
{
    // TODO @synoet implement if needed
}

void SAL_CALL ExpandedPackage::addVetoableChangeListener(const OUString& /*PropertyName*/, const uno::Reference<beans::XVetoableChangeListener>& /*aListener*/)
{
    // TODO @synoet implement if needed
}

void SAL_CALL ExpandedPackage::removeVetoableChangeListener(const OUString& /*PropertyName*/, const uno::Reference<beans::XVetoableChangeListener>& /*aListener*/)
{
    // TODO @synoet implement if needed
}

OUString SAL_CALL ExpandedPackage::getImplementationName()
{
    return "com.sun.star.comp.ExpandedPackage";
}

sal_Bool SAL_CALL ExpandedPackage::supportsService(const OUString& rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

uno::Sequence<OUString> SAL_CALL ExpandedPackage::getSupportedServiceNames()
{
    return { "com.sun.star.packages.ExpandedPackage" };
}

uno::Sequence<sal_Int8> ExpandedPackage::readFile(const OUString& path)
{
    ::osl::MutexGuard aGuard(m_aMutexHolder->GetMutex());

    for (const auto& rFile : m_aPackageFiles)
    {
        if (rFile.path == path)
            return rFile.content;
    }

    throw io::NotConnectedException(path, static_cast<cppu::OWeakObject*>(this));
}
