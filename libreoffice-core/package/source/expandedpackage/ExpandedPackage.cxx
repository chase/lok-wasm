#include <boost/property_tree/json_parser.hpp>
#include "ExpandedPackage.hxx"
#include "com/sun/star/beans/NamedValue.hdl"
#include "com/sun/star/io/XInputStream.hdl"
#include "com/sun/star/uno/Sequence.h"
#include "osl/thread.h"
#include <com/sun/star/io/NotConnectedException.hpp>
#include <com/sun/star/uno/Exception.hpp>
#include <cppuhelper/supportsservice.hxx>
#include <algorithm>
#include <memory>

using namespace com::sun::star;

ExpandedPackage::ExpandedPackage(uno::Reference<uno::XComponentContext> xContext)
    : m_aMutexHolder(new comphelper::RefCountedMutex)
    , m_nFormat(0)
    , m_xContext(std::move(xContext))
{
}

ExpandedPackage::~ExpandedPackage() = default;

std::vector<PackageFile> getPackageFilesFromInputStream(const uno::Reference<io::XInputStream> xInputStream)
{
    std::string rJson;

    if (xInputStream)
    {
        const sal_Int32 bufferSize = 4096;
        uno::Sequence<sal_Int8> buffer(bufferSize);
        sal_Int32 bytesRead;

        do
        {
            bytesRead = xInputStream->readBytes(buffer, bufferSize);
            rJson.append(reinterpret_cast<const char*>(buffer.getConstArray()), bytesRead);
        } while (bytesRead == bufferSize);
    }

    std::vector<PackageFile> parts;
    boost::property_tree::ptree aRootTree;
    boost::property_tree::read_json(rJson, aRootTree);
    for (const auto& part : boost::make_iterator_range(aRootTree))
    {
        auto path = part.second.get_value<std::string>("path");
        auto content = part.second.get_value<std::string>("content");
        // Generate SHA-256 hash
        const unsigned char* pData = reinterpret_cast<const unsigned char*>(content.data());

        uno::Sequence<sal_Int8> contentSequence(reinterpret_cast<const sal_Int8*>(pData), content.size());

        OUString sPath(path.data(), path.length(), osl_getThreadTextEncoding());


        parts.emplace_back(PackageFile(sPath,contentSequence));
    }

    return parts;

}

void SAL_CALL ExpandedPackage::initialize(const uno::Sequence<uno::Any>& aArguments)
{
    uno::Reference<io::XInputStream> pStream = nullptr;
    if ( !aArguments.hasElements() )
        return;
    bool bFound = false;
    for( const auto& rArgument : aArguments )
    {
        if ( rArgument >>= pStream )
        {
            auto parts = getPackageFilesFromInputStream(pStream);
            for (const PackageFile& part : parts)
            {
                m_aPackageFiles.insert_or_assign(part.path, part);
                bFound = true;
            }
        }
    }

    if (!bFound)
        throw uno::Exception("No input stream found", static_cast<cppu::OWeakObject*>(this));
}

uno::Any SAL_CALL ExpandedPackage::getByHierarchicalName(const OUString& aName)
{
    ::osl::MutexGuard aGuard(m_aMutexHolder->GetMutex());

    /* if (m_aPackageFiles.find(aName) != m_aPackageFiles.end()) */
    /* { */
    /*     return uno::Any(m_aPackageFiles.content); */
    /* } */


    throw container::NoSuchElementException(aName, static_cast<cppu::OWeakObject*>(this));
}

sal_Bool SAL_CALL ExpandedPackage::hasByHierarchicalName(const OUString& aName)
{
    ::osl::MutexGuard aGuard(m_aMutexHolder->GetMutex());
    if (m_aPackageFiles.find(aName) != m_aPackageFiles.end())
    {
        return true;
    }
    return false;
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

    if (m_aPackageFiles.find(path) != m_aPackageFiles.end())
    {

        return m_aPackageFiles.find(path)->second.content;

    }

    throw io::NotConnectedException(path, static_cast<cppu::OWeakObject*>(this));
}
