#include "com/sun/star/io/SequenceInputStream.hpp"
#include "com/sun/star/io/XInputStream.hdl"
#include "com/sun/star/io/XStream.hdl"
#include "com/sun/star/packages/NoEncryptionException.hdl"
#include "com/sun/star/uno/Reference.h"
#include "com/sun/star/uno/Sequence.h"
#include "comphelper/base64.hxx"
#include "comphelper/diagnose_ex.hxx"
#include "comphelper/seqstream.hxx"
#include "comphelper/sequence.hxx"
#include "oox/helper/binaryinputstream.hxx"
#include "oox/helper/binaryoutputstream.hxx"
#include "oox/helper/storagebase.hxx"
#include "sal/log.hxx"
#include "sot/stg.hxx"
#include "tools/stream.hxx"
#include "unotools/streamwrap.hxx"
#include <memory>
#include <oox/helper/expandedstorage.hxx>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/InvalidStorageException.hpp>
#include <emscripten/console.h>
#include <com/sun/star/embed/StorageWrappedTargetException.hpp>

#include <comphelper/ofopxmlhelper.hxx>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/io/XTruncate.hpp>
#include <comphelper/processfactory.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <osl/diagnose.h>
#include <string_view>

using namespace com::sun::star;

namespace oox
{

uno::Reference<io::XInputStream> SequenceStreamSupplier::getInputStream() { return m_xInput; }

uno::Reference<io::XOutputStream> SequenceStreamSupplier::getOutputStream() { return m_xOutput; }

ExpandedStorage::ExpandedStorage(const css::uno::Reference<uno::XComponentContext>& rxContext,
                                 const css::uno::Reference<css::io::XInputStream>& rxInStream)
    : StorageBase(rxInStream, false)
    , m_xContext(rxContext)
{
}

void ExpandedStorage::addPart(const std::string& path, const std::string& content)
{
    using namespace css::uno;
    OUString sPath = OUString::createFromAscii(path.c_str());
    Sequence<sal_Int8> sContent;
    comphelper::Base64::decode(sContent, OUString::createFromAscii(content.c_str()));
    ExpandedFile file(sPath, sContent);
    files.insert({ path, file });
}

// XInterface
css::uno::Any SAL_CALL ExpandedStorage::queryInterface(const css::uno::Type& rType)
{
    uno::Any aReturn = ::cppu::queryInterface(
        rType, static_cast<lang::XTypeProvider*>(this), static_cast<embed::XStorage*>(this),
        static_cast<embed::XHierarchicalStorageAccess*>(this),
        static_cast<container::XNameAccess*>(this), static_cast<container::XElementAccess*>(this),
        static_cast<lang::XComponent*>(this), static_cast<beans::XPropertySet*>(this),
        static_cast<embed::XRelationshipAccess*>(this));

    if (aReturn.hasValue())
        return aReturn;

    return OWeakObject::queryInterface(rType);
}

void SAL_CALL ExpandedStorage::acquire() noexcept { cppu::OWeakObject::acquire(); }

void SAL_CALL ExpandedStorage::release() noexcept { cppu::OWeakObject::release(); }

// XTypeProvider
css::uno::Sequence<css::uno::Type> SAL_CALL ExpandedStorage::getTypes()
{
    static css::uno::Sequence<css::uno::Type> aTypes = {
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<css::embed::XStorage>::get(),
        cppu::UnoType<css::embed::XHierarchicalStorageAccess>::get(),
        cppu::UnoType<css::beans::XPropertySet>::get(),
        cppu::UnoType<embed::XRelationshipAccess>::get(),
    };
    return aTypes;
}

css::uno::Sequence<sal_Int8> SAL_CALL ExpandedStorage::getImplementationId()
{
    return css::uno::Sequence<sal_Int8>();
}

// XStorage

/// Copies over all stream elements from this storage to the target storage.
void SAL_CALL ExpandedStorage::copyToStorage(const css::uno::Reference<css::embed::XStorage>& xDest)
{
    if (!xDest.is())
        throw css::uno::RuntimeException();

    for (const auto& [path, file] : files)
    {
        css::uno::Reference<css::io::XStream> xStream = xDest->openStreamElement(
            file.path, embed::ElementModes::READWRITE | embed::ElementModes::TRUNCATE);
        css::uno::Reference<css::io::XOutputStream> xOut = xStream->getOutputStream();
        xOut->writeBytes(file.content);
        xOut->closeOutput();
    }
}

css::uno::Reference<css::io::XStream>
    SAL_CALL ExpandedStorage::openStreamElement(const OUString& aStreamName, sal_Int32 nOpenMode)
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    auto it = files.find(std::string(aStreamName.toUtf8()));
    if (it == files.end())
        throw css::container::NoSuchElementException();

    const auto& file = it->second;

    uno::Reference<io::XInputStream> xInputStream(
        new comphelper::SequenceInputStream(file.content));
    uno::Reference<io::XStream> xStream = new SequenceStreamSupplier(xInputStream, nullptr);
    return xStream;
}

/// ExpandedStorage does not support encrypted streams, so this method is equivalent to openStreamElement.
css::uno::Reference<css::io::XStream> SAL_CALL
ExpandedStorage::openEncryptedStreamElement(const OUString&, sal_Int32, const OUString&)
{
    return openStreamElement(OUString(), 0);
}

/// ExpandedStorage is flat, so this method always returns itself.
css::uno::Reference<css::embed::XStorage>
    SAL_CALL ExpandedStorage::openStorageElement(const OUString&, sal_Int32)
{
    return this;
}

css::uno::Reference<css::io::XStream> SAL_CALL ExpandedStorage::cloneStreamElement(const OUString&)
{
    // TODO: @synoet - Implement this
    throw css::embed::StorageWrappedTargetException();
}

/// ExpandedStorage does not support encrypted streams, so this method is equivalent to cloneStreamElement.
css::uno::Reference<css::io::XStream>
    SAL_CALL ExpandedStorage::cloneEncryptedStreamElement(const OUString&, const OUString&)
{
    return cloneStreamElement(OUString());
}

void SAL_CALL
ExpandedStorage::copyLastCommitTo(const css::uno::Reference<css::embed::XStorage>& xTargetStorage)
{
    copyToStorage(xTargetStorage);
}

void SAL_CALL ExpandedStorage::copyStorageElementLastCommitTo(
    const OUString&, const css::uno::Reference<css::embed::XStorage>&)
{
    throw css::embed::InvalidStorageException();
}

sal_Bool SAL_CALL ExpandedStorage::isStreamElement(const OUString& aElementName)
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    return files.find(std::string(aElementName.toUtf8())) != files.end();
}

sal_Bool SAL_CALL ExpandedStorage::isStorageElement(const OUString&) { return sal_False; }

void SAL_CALL ExpandedStorage::removeElement(const OUString& aElementName)
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    files.erase(std::string(aElementName.toUtf8()));
}

void SAL_CALL ExpandedStorage::renameElement(const OUString& rEleName, const OUString& rNewName)
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    auto it = files.find(std::string(rEleName.toUtf8()));
    if (it == files.end())
        throw css::container::NoSuchElementException();

    auto nodeHandler = files.extract(it);
    nodeHandler.key() = rNewName.toUtf8();
    files.insert(std::move(nodeHandler));
}

void SAL_CALL ExpandedStorage::copyElementTo(const OUString& aElementName,
                                             const css::uno::Reference<css::embed::XStorage>& xDest,
                                             const OUString& aNewName)
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    auto it = files.find(std::string(aElementName.toUtf8()));
    if (it == files.end())
        throw css::container::NoSuchElementException();

    const auto& file = it->second;
    css::uno::Reference<css::io::XStream> xStream = xDest->openStreamElement(
        aNewName, embed::ElementModes::READWRITE | embed::ElementModes::TRUNCATE);
    css::uno::Reference<css::io::XOutputStream> xOut = xStream->getOutputStream();
    xOut->writeBytes(file.content);
    xOut->closeOutput();
}

void SAL_CALL ExpandedStorage::moveElementTo(const OUString& aElementName,
                                             const css::uno::Reference<css::embed::XStorage>& xDest,
                                             const OUString& rNewName)
{
    copyElementTo(aElementName, xDest, rNewName);
    removeElement(aElementName);
}

// XNameAccess
css::uno::Any SAL_CALL ExpandedStorage::getByName(const OUString& aName)
{
    std::lock_guard<std::mutex> lock(m_aMutex);

    uno::Any aResult;
    aResult <<= openStreamElement(aName, embed::ElementModes::READWRITE);

    return aResult;
}

css::uno::Sequence<OUString> SAL_CALL ExpandedStorage::getElementNames()
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    css::uno::Sequence<OUString> names(files.size());
    auto namesArray = names.getArray();
    size_t i = 0;
    for (const auto& pair : files)
    {
        namesArray[i++] = OUString::createFromAscii(pair.first.c_str());
    }
    return names;
}

sal_Bool SAL_CALL ExpandedStorage::hasByName(const OUString& aName)
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    return files.find(std::string(aName.toUtf8())) != files.end();
}

css::uno::Type SAL_CALL ExpandedStorage::getElementType() { return uno::Type(); }

sal_Bool SAL_CALL ExpandedStorage::hasElements()
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    return !files.empty();
}

// XPropertySet
css::uno::Reference<css::beans::XPropertySetInfo> SAL_CALL ExpandedStorage::getPropertySetInfo()
{
    return nullptr;
}

void SAL_CALL ExpandedStorage::setPropertyValue(const OUString&, const css::uno::Any&)
{
    throw css::beans::UnknownPropertyException();
}

css::uno::Any SAL_CALL ExpandedStorage::getPropertyValue(const OUString&)
{
    throw css::beans::UnknownPropertyException();
}

void SAL_CALL ExpandedStorage::addPropertyChangeListener(
    const OUString&, const css::uno::Reference<css::beans::XPropertyChangeListener>&)
{
}

void SAL_CALL ExpandedStorage::removePropertyChangeListener(
    const OUString&, const css::uno::Reference<css::beans::XPropertyChangeListener>&)
{
}

void SAL_CALL ExpandedStorage::addVetoableChangeListener(
    const OUString&, const css::uno::Reference<css::beans::XVetoableChangeListener>&)
{
}

void SAL_CALL ExpandedStorage::removeVetoableChangeListener(
    const OUString&, const css::uno::Reference<css::beans::XVetoableChangeListener>&)
{
}

// XHierarchicalStorageAccess
css::uno::Reference<css::embed::XExtendedStorageStream>
    SAL_CALL ExpandedStorage::openStreamElementByHierarchicalName(const OUString& sStreamPath,
                                                                  sal_Int32 nOpenMode)
{
    SAL_WARN("expandedstorage", "openStreamElementByHierarchicalName: " << sStreamPath);
    uno::Reference<embed::XExtendedStorageStream> xResult;
    uno::Reference<io::XStream> xStream = openStreamElement(sStreamPath, nOpenMode);

    uno::Reference<io::XInputStream> xInputStream = xStream->getInputStream();

    xResult.set(xInputStream, uno::UNO_QUERY);
    return xResult;
}

css::uno::Reference<css::embed::XExtendedStorageStream> SAL_CALL
ExpandedStorage::openEncryptedStreamElementByHierarchicalName(const OUString& sStreamPath,
                                                              sal_Int32 nOpenMode, const OUString&)
{
    return openStreamElementByHierarchicalName(sStreamPath, nOpenMode);
}

void SAL_CALL ExpandedStorage::removeStreamElementByHierarchicalName(const OUString& sElementPath)
{
    removeElement(sElementPath);
}

//  XDisposable
void SAL_CALL ExpandedStorage::dispose()
{
    std::unique_lock aGuard(m_aMutex);
    disposeImpl(aGuard);
}

void ExpandedStorage::disposeImpl(std::unique_lock<std::mutex>& rGuard)
{
    if (m_aListenersContainer.getLength(rGuard))
    {
        lang::EventObject aSource(getXWeak());
        m_aListenersContainer.disposeAndClear(rGuard, aSource);
    }
}

void SAL_CALL
ExpandedStorage::addEventListener(const uno::Reference<lang::XEventListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);

    m_aListenersContainer.addInterface(aGuard, xListener);
}

void SAL_CALL
ExpandedStorage::removeEventListener(const uno::Reference<lang::XEventListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);

    m_aListenersContainer.removeInterface(aGuard, xListener);
}

// StorageBase
bool ExpandedStorage::implIsStorage() const { return true; }

css::uno::Reference<css::embed::XStorage> ExpandedStorage::implGetXStorage() const
{
    return css::uno::Reference<css::embed::XStorage>(const_cast<ExpandedStorage*>(this));
}

void ExpandedStorage::implGetElementNames(::std::vector<OUString>& orElementNames) const
{
    size_t i = 0;
    for (const auto& pair : files)
    {
        orElementNames[i++] = OUString::createFromAscii(pair.first.c_str());
    }
}

StorageRef ExpandedStorage::implOpenSubStorage(const OUString&, bool)
{
    return std::shared_ptr<StorageBase>(std::move(this));
}

css::uno::Reference<css::io::XInputStream>
ExpandedStorage::implOpenInputStream(const OUString& rElementName)
{
    return openStreamElement(rElementName, embed::ElementModes::READ)->getInputStream();
}

css::uno::Reference<css::io::XOutputStream>
ExpandedStorage::implOpenOutputStream(const OUString& rElementName)
{
    return openStreamElement(rElementName, embed::ElementModes::READWRITE)->getOutputStream();
}

void ExpandedStorage::implCommit() const {}

const beans::StringPair* lcl_findPairByName(const uno::Sequence<beans::StringPair>& rSeq,
                                            const OUString& rName)
{
    return std::find_if(rSeq.begin(), rSeq.end(),
                        [&rName](const beans::StringPair& rPair) { return rPair.First == rName; });
}

void ExpandedStorage::readRelationshipInfo()
{
    uno::Reference<io::XInputStream> xRelInfoStream
        = openStreamElement("_rels/.rels", embed::ElementModes::READ)->getInputStream();
    SAL_WARN("expandedstorage", "readRelationshipInfo: " << xRelInfoStream.is() << " available "
                                                         << xRelInfoStream->available());
    m_aRelInfo = ::comphelper::OFOPXMLHelper::ReadRelationsInfoSequence(xRelInfoStream,
                                                                        u"_rels/.rels", m_xContext);
}

void SAL_CALL ExpandedStorage::clearRelationships() { m_aRelInfo.realloc(0); }

void SAL_CALL ExpandedStorage::insertRelationships(
    const css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>& aEntries,
    sal_Bool bReplace)
{
    OUString aIDTag("Id");
    const uno::Sequence<uno::Sequence<beans::StringPair>> aSeq = getAllRelationships();
    std::vector<uno::Sequence<beans::StringPair>> aResultVec;
    aResultVec.reserve(aSeq.getLength() + aEntries.getLength());

    std::copy_if(aSeq.begin(), aSeq.end(), std::back_inserter(aResultVec),
                 [&aIDTag, &aEntries](const uno::Sequence<beans::StringPair>& rTargetRel)
                 {
                     auto pTargetPair = lcl_findPairByName(rTargetRel, aIDTag);
                     if (pTargetPair == rTargetRel.end())
                         return false;

                     bool bIsSourceSame = std::any_of(
                         aEntries.begin(), aEntries.end(),
                         [&pTargetPair](const uno::Sequence<beans::StringPair>& rSourceEntry)
                         {
                             return std::find(rSourceEntry.begin(), rSourceEntry.end(),
                                              *pTargetPair)
                                    != rSourceEntry.end();
                         });

                     // if no such element in the provided sequence
                     return !bIsSourceSame;
                 });

    std::transform(
        aEntries.begin(), aEntries.end(), std::back_inserter(aResultVec),
        [&aIDTag](
            const uno::Sequence<beans::StringPair>& rEntry) -> uno::Sequence<beans::StringPair>
        {
            auto pPair = lcl_findPairByName(rEntry, aIDTag);

            auto aResult = comphelper::sequenceToContainer<std::vector<beans::StringPair>>(rEntry);
            auto nIDInd = std::distance(rEntry.begin(), pPair);
            std::rotate(aResult.begin(), std::next(aResult.begin(), nIDInd),
                        std::next(aResult.begin(), nIDInd + 1));

            return comphelper::containerToSequence(aResult);
        });

    m_aRelInfo = comphelper::containerToSequence(aResultVec);
}

void SAL_CALL ExpandedStorage::removeRelationshipByID(const OUString& sID)
{
    uno::Sequence<uno::Sequence<beans::StringPair>> aSeq = getAllRelationships();
    const beans::StringPair aIDRel("Id", sID);
    auto pRel = std::find_if(std::cbegin(aSeq), std::cend(aSeq),
                             [&aIDRel](const uno::Sequence<beans::StringPair>& rRel)
                             { return std::find(rRel.begin(), rRel.end(), aIDRel) != rRel.end(); });
    if (pRel != std::cend(aSeq))
    {
        auto nInd = static_cast<sal_Int32>(std::distance(std::cbegin(aSeq), pRel));
        comphelper::removeElementAt(aSeq, nInd);

        m_aRelInfo = aSeq;

        // TODO/LATER: in future the unification of the ID could be checked
        return;
    }

    throw container::NoSuchElementException();
}
void SAL_CALL ExpandedStorage::insertRelationshipByID(
    const OUString& sID, const uno::Sequence<beans::StringPair>& aEntry, sal_Bool bReplace)
{
    const beans::StringPair aIDRel("Id", sID);

    uno::Sequence<beans::StringPair>* pResult = nullptr;

    // TODO/LATER: in future the unification of the ID could be checked
    uno::Sequence<uno::Sequence<beans::StringPair>> aSeq = getAllRelationships();
    for (sal_Int32 nInd = 0; nInd < aSeq.getLength(); nInd++)
    {
        const auto& rRel = aSeq[nInd];
        if (std::find(rRel.begin(), rRel.end(), aIDRel) != rRel.end())
            pResult = &aSeq.getArray()[nInd];
    }

    if (!pResult)
    {
        const sal_Int32 nIDInd = aSeq.getLength();
        aSeq.realloc(nIDInd + 1);
        pResult = &aSeq.getArray()[nIDInd];
    }

    std::vector<beans::StringPair> aResult;
    aResult.reserve(aEntry.getLength() + 1);

    aResult.push_back(aIDRel);
    std::copy_if(aEntry.begin(), aEntry.end(), std::back_inserter(aResult),
                 [](const beans::StringPair& rPair) { return rPair.First != "Id"; });

    *pResult = comphelper::containerToSequence(aResult);

    m_aRelInfo = aSeq;
}

uno::Sequence<uno::Sequence<beans::StringPair>>
    SAL_CALL ExpandedStorage::getRelationshipsByType(const OUString& sType)
{
    const uno::Sequence<uno::Sequence<beans::StringPair>> aSeq = getAllRelationships();
    std::vector<uno::Sequence<beans::StringPair>> aResult;
    aResult.reserve(aSeq.getLength());

    std::copy_if(aSeq.begin(), aSeq.end(), std::back_inserter(aResult),
                 [&sType](const uno::Sequence<beans::StringPair>& rRel)
                 {
                     auto pRel = lcl_findPairByName(rRel, "Type");
                     return pRel != rRel.end()
                            // the type is usually a URL, so the check should be case insensitive
                            && pRel->Second.equalsIgnoreAsciiCase(sType);
                 });

    return comphelper::containerToSequence(aResult);
}

OUString SAL_CALL ExpandedStorage::getTypeByID(const OUString& sID)
{
    const uno::Sequence<beans::StringPair> aSeq = getRelationshipByID(sID);
    auto pRel = lcl_findPairByName(aSeq, "Type");
    if (pRel != aSeq.end())
        return pRel->Second;

    return OUString();
}

OUString SAL_CALL ExpandedStorage::getTargetByID(const OUString& sID)
{
    const uno::Sequence<beans::StringPair> aSeq = getRelationshipByID(sID);
    auto pRel = lcl_findPairByName(aSeq, "Target");
    if (pRel != aSeq.end())
        return pRel->Second;

    return OUString();
}

sal_Bool SAL_CALL ExpandedStorage::hasByID(const OUString& sID)
{
    try
    {
        getRelationshipByID(sID);
        return true;
    }
    catch (const container::NoSuchElementException&)
    {
        TOOLS_INFO_EXCEPTION("package.xstor", "Rethrow:");
    }

    return false;
}

uno::Sequence<beans::StringPair> SAL_CALL ExpandedStorage::getRelationshipByID(const OUString& sID)
{
    const uno::Sequence<uno::Sequence<beans::StringPair>> aSeq = getAllRelationships();
    const beans::StringPair aIDRel("Id", sID);

    auto pRel = std::find_if(aSeq.begin(), aSeq.end(),
                             [&aIDRel](const uno::Sequence<beans::StringPair>& rRel)
                             { return std::find(rRel.begin(), rRel.end(), aIDRel) != rRel.end(); });
    if (pRel != aSeq.end())
        return *pRel;

    throw container::NoSuchElementException();
}

uno::Sequence<uno::Sequence<beans::StringPair>> SAL_CALL ExpandedStorage::getAllRelationships()
{
    return m_aRelInfo;
}

} // namespace oox
