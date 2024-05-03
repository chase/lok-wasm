#include "comphelper/hash.hxx"
#include "osl/thread.h"
#include <cstddef>
#include <oox/helper/expandedstorage.hxx>

#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <osl/diagnose.h>
#include <sal/log.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <comphelper/storagehelper.hxx>

namespace oox {

using namespace ::com::sun::star::container;
using namespace ::com::sun::star::embed;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;

OUString toHexString(const std::vector<unsigned char>& a)
{
    std::stringstream aStrm;
    for (auto& i : a)
    {
        aStrm << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(i);
    }


    std::string str = aStrm.str();

    return OUString(str.data(), str.length(), osl_getThreadTextEncoding());
}

std::vector<ExpandedPart> GetExpandedParts(Reference<XStorage> storage, std::optional<OUString> &path)
{
    std::vector<ExpandedPart> expandedParts;
    Sequence<OUString> elementNames = storage->getElementNames();
    for (const OUString& name : elementNames)
    {
        const OUString& fullPath = path.value_or("") + "/" + name;

        // Indicates that the element is a directory
        if (storage->isStorageElement(name))
        {
            Reference<XStorage> subStorage = storage->openStorageElement(name, ElementModes::READ);
            std::optional<OUString> subPath = std::make_optional<OUString>(fullPath);
            // Recursively initialize the expanded parts
            std::vector<ExpandedPart> subParts = GetExpandedParts(subStorage, subPath);

            for (ExpandedPart part : subParts) {
                expandedParts.push_back(part);
            }
        }
        // Indicates that the element is a streamable file
        else if (storage->isStreamElement(name))
        {
            // Double check that the path exists
            // before trying to open the stream element
            // trying to open a non-existent path will hang indefinately
            if (!storage->hasByName(name))
            {
                SAL_WARN("desktop", "ExpandedStorage::ExpandedStorage() - path: " << fullPath << " does not exist");
                continue;
            }

            Reference<XStream> xStream = storage->openStreamElement(name, ElementModes::READ);

            if (!xStream.is())
            {
                SAL_WARN("desktop", "ExpandedStorage::ExpandedStorage() - xStream for path " << name << " is null");
                continue;
            }

            Reference<XInputStream> inputStream = xStream->getInputStream();

            if (!inputStream.is())
            {
                SAL_WARN("desktop", "ExpandedStorage::ExpandedStorage() - inputStream for path " << fullPath << " is null");
                continue;
            }

            // Read the file content
            Sequence<sal_Int8> aFileContent;
            inputStream->readBytes(aFileContent, inputStream->available());
            const sal_Int8* pData = aFileContent.getConstArray();
            const unsigned char* pUnsignedData = reinterpret_cast<const unsigned char*>(pData);

            // Generate SHA-256 hash
            std::vector<unsigned char> hashVec = comphelper::Hash::calculateHash(
                pUnsignedData, aFileContent.getLength(),
                comphelper::HashType::SHA256
            );

            const OUString& hashString = toHexString(hashVec);


            ExpandedPart part(fullPath, hashString, pUnsignedData);
            expandedParts.push_back(part);
        }
    }

    return expandedParts;
}

ExpandedStorage::ExpandedStorage( const Reference< XComponentContext >& rxContext, const Reference< XInputStream >& rxInStream, bool bRepairStorage ) :
    StorageBase( rxInStream, false )
{
    if( !rxContext.is() )
        return;

    uno::Reference<embed::XStorage> storage = ::comphelper::OStorageHelper::GetStorageOfFormatFromInputStream(
            ZIP_STORAGE_FORMAT_STRING, rxInStream, rxContext, bRepairStorage);

    auto path = std::optional<OUString>();
    std::vector<ExpandedPart> parts = GetExpandedParts(storage, path);

    for (ExpandedPart part : parts) {
        addPart(part);
    }
}
ExpandedStorage::ExpandedStorage( std::vector<ExpandedPart>& parts, bool bRepairStorage) :
    StorageBase()
{
    for (ExpandedPart part : parts) {
        addPart(part);
    }
}


bool ExpandedStorage::implIsStorage() const
{
    return true;
}

Reference< XStorage > ExpandedStorage::implGetXStorage() const
{
    return nullptr;
}

void ExpandedStorage::implGetElementNames( ::std::vector< OUString >& orElementNames ) const
{
    for (const std::pair<const OUString, ExpandedPart>& p : m_parts)
    {
        orElementNames.push_back(p.first);
    }
}

StorageRef ExpandedStorage::implOpenSubStorage( const OUString& rElementName, bool bCreateMissing )
{
    return std::shared_ptr<ExpandedStorage>(this);
}

Reference< XInputStream > ExpandedStorage::implOpenInputStream( const OUString& rElementName )
{
    Reference< XInputStream > xInStream;

    if (m_parts.find(rElementName) == m_parts.end())
    {
        return nullptr;
    }

    auto found = m_parts.find(rElementName);

    ExpandedPart& foundPart = found->second;

    Sequence<sal_Int8> aFileContent(reinterpret_cast<signed char*>(foundPart.content));

    xInStream->readBytes(foundPart.content, foundPart.size);






    xInStream.set( mxStorage->openStreamElement( rElementName, css::embed::ElementModes::READ ), UNO_QUERY );
    catch (Exception const&)
    {
        TOOLS_INFO_EXCEPTION("oox.storage", "openStreamElement");
    }
    return xInStream;
}

Reference< XOutputStream > ZipStorage::implOpenOutputStream( const OUString& rElementName )
{
    Reference< XOutputStream > xOutStream;
    if( mxStorage.is() ) try
    {
        xOutStream.set( mxStorage->openStreamElement( rElementName, css::embed::ElementModes::READWRITE ), UNO_QUERY );
    }
    catch (Exception const&)
    {
        TOOLS_INFO_EXCEPTION("oox.storage", "openStreamElement");
    }
    return xOutStream;
}

void ZipStorage::implCommit() const
{
    try
    {
        Reference< XTransactedObject >( mxStorage, UNO_QUERY_THROW )->commit();
    }
    catch (Exception const&)
    {
        TOOLS_WARN_EXCEPTION("oox.storage", "commit");
    }
}

} // namespace oox

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
