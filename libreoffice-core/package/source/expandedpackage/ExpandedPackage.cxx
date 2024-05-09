#include <boost/property_tree/json_parser.hpp>
#include "ExpandedPackage.hxx"
#include "com/sun/star/io/XInputStream.hdl"
#include "com/sun/star/io/XStream.hdl"
#include "com/sun/star/uno/Sequence.h"
#include "com/sun/star/uno/XInterface.hdl"
#include "osl/thread.h"
#include "sal/log.hxx"
#include <com/sun/star/io/NotConnectedException.hpp>
#include <com/sun/star/uno/Exception.hpp>
#include <cppuhelper/supportsservice.hxx>

using namespace com::sun::star;

ExpandedPackage::ExpandedPackage(uno::Reference<uno::XComponentContext> xContext)
    : m_aMutexHolder(new comphelper::RefCountedMutex)
    , m_nFormat(0)
    , m_xContext(std::move(xContext))
{

    SAL_WARN("expandedpackage", "initializing");
}

ExpandedPackage::~ExpandedPackage() = default;

std::vector<PackageFile> getPackageFilesFromInputStream(const uno::Reference<io::XInputStream> xInputStream)
{
    std::vector<PackageFile> parts;
    std::string rJson;

    if (!xInputStream)
    {
        SAL_WARN("expandedpackage", "no input stream");
        return parts;
    }

    if (xInputStream->available() == 0)
    {
        SAL_WARN("expandedpackage", "INPUT STREAM IS EMPTY");
        return parts;
    }

    const sal_Int32 bufferSize = 4096;
    uno::Sequence<sal_Int8> buffer(bufferSize);
    sal_Int32 bytesRead;


    do
    {
        bytesRead = xInputStream->readBytes(buffer, bufferSize);
        rJson.append(reinterpret_cast<const char*>(buffer.getConstArray()), bytesRead);
    } while (bytesRead == bufferSize);

    boost::property_tree::ptree aRootTree;
    std::stringstream aStream(rJson);
    boost::property_tree::read_json(aStream, aRootTree);

    auto child = aRootTree.get_child("parts");
    for (const auto& part : boost::make_iterator_range(child))
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
    uno::Reference<io::XInputStream> xInputStream = nullptr;
    uno::Reference<io::XStream> xStream = nullptr;
    if ( !aArguments.hasElements() )
        return;
    bool bFound = false;
    for( const auto& rArgument : aArguments )
    {
        if ( rArgument >>= xInputStream )
        {
            auto parts = getPackageFilesFromInputStream(xInputStream);

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

    if (m_aPackageFiles.find(aName) != m_aPackageFiles.end())
    {
        const PackageFile& foundFile = m_aPackageFiles.at(aName);
        std::string content = reinterpret_cast<const char*>(foundFile.content.getConstArray());

        return uno::toAny(OUString(content.c_str(), content.size(), osl_getThreadTextEncoding()));
    }


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
uno::Reference< uno::XInterface > SAL_CALL ExpandedPackage::createInstance()
{
    uno::Reference < XInterface > xRef = *this;
    return xRef;
}

uno::Reference< uno::XInterface > SAL_CALL ExpandedPackage::createInstanceWithArguments(const css::uno::Sequence<css::uno::Any>& aArguments)
{
    uno::Reference < XInterface > xRef = *this;
    return xRef;
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
    return { "com.sun.star.packages.Package" };
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

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
package_ExpandedPackage_get_implementation(
    css::uno::XComponentContext* context , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new ExpandedPackage(context));
}
