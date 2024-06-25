#include "com/sun/star/beans/UnknownPropertyException.hdl"
#include "com/sun/star/beans/XPropertySetInfo.hdl"
#include "com/sun/star/embed/ElementModes.hdl"
#include "com/sun/star/embed/XExtendedStorageStream.hdl"
#include "com/sun/star/embed/XStorage.hdl"
#include "com/sun/star/io/XInputStream.hdl"
#include "com/sun/star/io/XStream.hdl"
#include "com/sun/star/packages/NoEncryptionException.hdl"
#include "com/sun/star/uno/Reference.h"
#include "com/sun/star/uno/Sequence.h"
#include "comphelper/base64.hxx"
#include "comphelper/diagnose_ex.hxx"
#include "comphelper/hash.hxx"
#include "comphelper/seqstream.hxx"
#include "comphelper/vecstream.hxx"
#include "comphelper/sequence.hxx"
#include "oox/helper/binaryinputstream.hxx"
#include "oox/helper/binaryoutputstream.hxx"
#include "oox/helper/storagebase.hxx"
#include "osl/thread.h"
#include "sal/log.hxx"
#include "sot/stg.hxx"
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

using namespace com::sun::star::uno;

namespace oox
{

namespace helpers
{

OUString toOUString(const std::string& value) { return OUString::createFromAscii(value.c_str()); }

std::string toString(const OUString& value) { return std::string(value.toUtf8()); }

std::vector<std::string> split(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter))
    {
        tokens.push_back(token);
    }

    return tokens;
}

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

OUString getContentHash(const std::vector<sal_Int8>& content)
{
    const unsigned char* unsignedData = reinterpret_cast<const unsigned char*>(content.data());

    // Generate SHA-256 hash
    std::vector<unsigned char> hashVec = comphelper::Hash::calculateHash(
        unsignedData, content.size(), comphelper::HashType::SHA256);

    return toHexString(hashVec);
}
}

SequenceStreamSupplier::SequenceStreamSupplier(Reference<io::XInputStream> xInput,
                                               Reference<io::XOutputStream> xOutput)
    : m_xInput(std::move(xInput))
    , m_xOutput(std::move(xOutput))
{
    m_xSeekable.set(m_xInput, uno::UNO_QUERY);
    if (!m_xSeekable.is())
        m_xSeekable.set(m_xOutput, uno::UNO_QUERY);
}

Reference<io::XInputStream> SAL_CALL SequenceStreamSupplier::getInputStream() { return m_xInput; }

Reference<io::XOutputStream> SAL_CALL SequenceStreamSupplier::getOutputStream()
{
    return m_xOutput;
}

void SAL_CALL SequenceStreamSupplier::seek(sal_Int64 nLocation)
{
    if (!m_xSeekable.is())
        throw io::NotConnectedException();
    m_xSeekable->seek(nLocation);
}

sal_Int64 SAL_CALL SequenceStreamSupplier::getPosition()
{
    if (!m_xSeekable.is())
        throw io::NotConnectedException();
    return m_xSeekable->getPosition();
}

sal_Int64 SAL_CALL SequenceStreamSupplier::getLength()
{
    if (!m_xSeekable.is())
        throw io::NotConnectedException();

    return m_xSeekable->getLength();
}

SequenceStreamContainer::SequenceStreamContainer(Reference<io::XStream>& xStream)
    : m_xStream(xStream)
{
}

Reference<io::XInputStream> SAL_CALL SequenceStreamContainer::getInputStream()
{
    return m_xStream->getInputStream();
}
Reference<io::XOutputStream> SAL_CALL SequenceStreamContainer::getOutputStream()
{
    return m_xStream->getOutputStream();
}

Any SAL_CALL SequenceStreamContainer::queryInterface(const Type& rType)
{
    Any aRet = cppu::queryInterface(rType, static_cast<embed::XExtendedStorageStream*>(this),
                                    static_cast<io::XStream*>(this));
    if (aRet.hasValue())
        return aRet;

    return OWeakObject::queryInterface(rType);
}

void SAL_CALL SequenceStreamContainer::acquire() noexcept { OWeakObject::acquire(); }

void SAL_CALL SequenceStreamContainer::release() noexcept { OWeakObject::release(); }

void SAL_CALL SequenceStreamContainer::dispose()
{
    std::unique_lock aGuard(m_aMutex);
    if (m_aListenersContainer.getLength(aGuard))
    {
        lang::EventObject aSource(getXWeak());
        m_aListenersContainer.disposeAndClear(aGuard, aSource);
    }
}

void SAL_CALL
SequenceStreamContainer::addEventListener(const Reference<lang::XEventListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);

    m_aListenersContainer.addInterface(aGuard, xListener);
}

void SAL_CALL
SequenceStreamContainer::removeEventListener(const Reference<lang::XEventListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);

    m_aListenersContainer.removeInterface(aGuard, xListener);
}

ExpandedStorage::ExpandedStorage(const Reference<XComponentContext>& rxContext,
                                 const Reference<io::XInputStream>& rxInStream)
    : StorageBase(rxInStream, false, false)
    , m_xContext(rxContext)
    , m_inputStream(rxInStream)
{
}

OUString ExpandedStorage::getFullPath(const OUString& path) const
{
    return helpers::toOUString(m_basePath.value_or("")) + "/" + path;
}

ExpandedStorage::ExpandedStorage(const Reference<XComponentContext>& context_,
                                 const std::shared_ptr<ExpandedFileMap>& fileMap_,
                                 const Reference<io::XInputStream>& inputStream_,
                                 const OUString& basePath_)
    : StorageBase(inputStream_, false, false)
    , m_files(fileMap_)
    , m_xContext(context_)
    , m_basePath(helpers::toString(basePath_))
    , m_inputStream(inputStream_)
{
}

void ExpandedStorage::addPart(const std::string& path, const std::string& content)
{
    std::vector<std::string> pathParts = helpers::split(path, '/');
    for (int idx = 0; idx < pathParts.size() - 1; idx++)
    {
        bool found = std::find(m_dirs.begin(), m_dirs.end(), pathParts[idx]) != m_dirs.end();
        if (!found)
        {
            m_dirs.push_back(pathParts[idx]);
        }
    }
    OUString sPath = OUString::createFromAscii(path.c_str());
    Sequence<sal_Int8> seqContent;
    comphelper::Base64::decode(seqContent, helpers::toOUString(content));
    std::vector<sal_Int8> fileContent = comphelper::sequenceToContainer<std::vector<sal_Int8>>(seqContent);
    OUString sha = helpers::getContentHash(fileContent);

    seqContent.realloc(0);

    SAL_WARN("expandedstorage", "addPart: sha " << sha << " path " << path << " content size " << fileContent.size());

    m_files->insert({ path, { sPath, fileContent, sha } });
}

std::optional<std::pair<std::string, std::string>>
ExpandedStorage::getPart(const std::string& path) const
{
    auto it = std::find_if(m_files->begin(), m_files->end(),
                           [&path](const auto& pair)
                           {
                               return pair.first == path;
                           });

    if (it == m_files->end())
    {
        SAL_WARN("expandedstorage", "getPart: part not found " << path);
        return {};
    }

    std::string content(*it->second.content.data(), it->second.content.size());

    return std::make_pair(helpers::toString(it->second.path), content);
}

void ExpandedStorage::removePart(const std::string& path)
{
    if (getPart(path).has_value())
    {
        SAL_WARN("expandedstorage", "removePart: part not found" << path);
        return;
    }

    m_files->erase(path);
}

std::vector<std::pair<const std::string, const std::string>> ExpandedStorage::listParts()
{

    for (auto& [path, file] : *m_files)
    {
        file.sha = helpers::getContentHash(file.content);
    }
    std::vector<std::pair<const std::string, const std::string>> parts;
    for (const auto& [path, file] : *m_files)
    {
        const std::string pathString = helpers::toString(file.path);
        const std::string shaString = helpers::toString(file.sha);

        parts.push_back({ pathString, shaString });
    }

    if (parts.empty())
    {
        SAL_WARN("expandedstorage", "listParts: no parts found");
    }

    return parts;
}

// XInterface
Any SAL_CALL ExpandedStorage::queryInterface(const Type& rType)
{
    Any aReturn = ::cppu::queryInterface(
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
Sequence<Type> SAL_CALL ExpandedStorage::getTypes()
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

Sequence<sal_Int8> SAL_CALL ExpandedStorage::getImplementationId() { return Sequence<sal_Int8>(); }

// XStorage

/// Copies over all stream elements from this storage to the target storage.
void SAL_CALL ExpandedStorage::copyToStorage(const Reference<embed::XStorage>& xDest)
{
    if (!xDest.is())
        throw RuntimeException();

    for (const auto& [path, file] : *m_files)
    {
        Reference<io::XStream> xStream = xDest->openStreamElement(
            file.path, embed::ElementModes::READWRITE | embed::ElementModes::TRUNCATE);
        Reference<io::XOutputStream> xOut = xStream->getOutputStream();
        xOut->writeBytes(comphelper::containerToSequence(file.content));
        xOut->closeOutput();
    }
}
bool shouldCreateStreamElement(sal_Int32 openMode)
{
    return (openMode != embed::ElementModes::READ) &&
        (openMode != embed::ElementModes::NOCREATE) &&
        (openMode != embed::ElementModes::SEEKABLEREAD);

}
Reference<io::XStream> ExpandedStorage::openStreamElement(const OUString& name, sal_Int32 nOpenMode,
                                                          PathType pathType)
{

    std::string path = pathType == PathType::Absolute ? helpers::toString(name)
                                                      : helpers::toString(getFullPath(name));
    std::lock_guard<std::mutex> lock(m_aMutex);
    auto it = m_files->find(std::string(path));
    if (it == m_files->end())
    {
        if (!shouldCreateStreamElement(nOpenMode))
        {

            throw css::container::NoSuchElementException();
        }

        it = m_files->insert({path, {name, std::vector<sal_Int8>(), ""}}).first;
    }

    auto& file = it->second;

    Reference<io::XInputStream> maybeInputStream;
    Reference<io::XOutputStream> maybeOutputStream;

    if (nOpenMode == embed::ElementModes::READWRITE ||
        nOpenMode == embed::ElementModes::READ ||
        nOpenMode == embed::ElementModes::SEEKABLE ||
        nOpenMode == embed::ElementModes::SEEKABLEREAD)
    {
        Reference<io::XInputStream> inputStream(new comphelper::VectorInputStream(file.content));
        maybeInputStream = inputStream;
    }

    if (nOpenMode == embed::ElementModes::READWRITE ||
        nOpenMode == embed::ElementModes::WRITE ||
        nOpenMode == embed::ElementModes::TRUNCATE)
    {
        Reference<io::XOutputStream> outputStream(new comphelper::VectorOutputStream(file.content));
        maybeOutputStream = outputStream;

        m_staleSha = true;
    }

    Reference<io::XStream> xStream = new SequenceStreamSupplier(maybeInputStream, maybeOutputStream);
    return xStream;
}

// name is relative
Reference<io::XStream> SAL_CALL ExpandedStorage::openStreamElement(const OUString& name,
                                                                   sal_Int32 openMode)
{
    return openStreamElement(name, openMode, PathType::Relative);
}

/// ExpandedStorage does not support encrypted streams, so this method is equivalent to openStreamElement.
Reference<io::XStream> SAL_CALL ExpandedStorage::openEncryptedStreamElement(const OUString&,
                                                                            sal_Int32,
                                                                            const OUString&)
{
    return openStreamElement(OUString(), 0);
}

/// ExpandedStorage is flat, so this method always returns itself.
Reference<embed::XStorage> SAL_CALL ExpandedStorage::openStorageElement(const OUString& path,
                                                                        sal_Int32 openMode)
{
    if (path == "/")
    {
        return this;
    }

    Reference<ExpandedStorage> expandedStorage(
        new ExpandedStorage(m_xContext, m_files, m_inputStream, path));
    return Reference<embed::XStorage>(expandedStorage);
}

Reference<io::XStream> SAL_CALL ExpandedStorage::cloneStreamElement(const OUString&)
{
    SAL_WARN("expandedstorage", "cloneStreamElement: not implemented");
    // TODO: @synoet - Implement this
    throw css::embed::StorageWrappedTargetException();
}

/// ExpandedStorage does not support encrypted streams, so this method is equivalent to cloneStreamElement.
Reference<io::XStream> SAL_CALL ExpandedStorage::cloneEncryptedStreamElement(const OUString&,
                                                                             const OUString&)
{
    return cloneStreamElement(OUString());
}

void SAL_CALL ExpandedStorage::copyLastCommitTo(const Reference<embed::XStorage>& xTargetStorage)
{
    copyToStorage(xTargetStorage);
}

void SAL_CALL ExpandedStorage::copyStorageElementLastCommitTo(const OUString&,
                                                              const Reference<embed::XStorage>&)
{
    throw embed::InvalidStorageException();
}

sal_Bool SAL_CALL ExpandedStorage::isStreamElement(const OUString& aElementName)
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    return m_files->find(helpers::toString(getFullPath(aElementName))) != m_files->end();
}

sal_Bool SAL_CALL ExpandedStorage::isStorageElement(const OUString& path)
{
    return std::find(m_dirs.begin(), m_dirs.end(), helpers::toString(path)) != m_dirs.end();
}

void SAL_CALL ExpandedStorage::removeElement(const OUString& aElementName)
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    m_files->erase(std::string(aElementName.toUtf8()));
}

void SAL_CALL ExpandedStorage::renameElement(const OUString& rEleName, const OUString& rNewName)
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    auto it = m_files->find(std::string(rEleName.toUtf8()));
    if (it == m_files->end())
        throw container::NoSuchElementException();

    auto nodeHandler = m_files->extract(it);
    nodeHandler.key() = rNewName.toUtf8();
    m_files->insert(std::move(nodeHandler));
}

void SAL_CALL ExpandedStorage::copyElementTo(const OUString& aElementName,
                                             const Reference<css::embed::XStorage>& xDest,
                                             const OUString& aNewName)
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    auto it = m_files->find(std::string(aElementName.toUtf8()));
    if (it == m_files->end())
        throw css::container::NoSuchElementException();

    const auto& file = it->second;
    Reference<io::XStream> xStream = xDest->openStreamElement(
        aNewName, embed::ElementModes::READWRITE | embed::ElementModes::TRUNCATE);
    Reference<io::XOutputStream> xOut = xStream->getOutputStream();
    xOut->writeBytes(comphelper::containerToSequence(file.content));
    xOut->closeOutput();
}

void SAL_CALL ExpandedStorage::moveElementTo(const OUString& aElementName,
                                             const Reference<css::embed::XStorage>& xDest,
                                             const OUString& rNewName)
{
    copyElementTo(aElementName, xDest, rNewName);
    removeElement(aElementName);
}

// XNameAccess

/// name is relative path to the current storage
css::uno::Any SAL_CALL ExpandedStorage::getByName(const OUString& name)
{
    std::lock_guard<std::mutex> lock(m_aMutex);

    uno::Any aResult;
    aResult <<= openStreamElement(getFullPath(name), embed::ElementModes::READWRITE);

    return aResult;
}

css::uno::Sequence<OUString> SAL_CALL ExpandedStorage::getElementNames()
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    css::uno::Sequence<OUString> names(m_files->size());
    auto namesArray = names.getArray();
    size_t i = 0;
    for (const auto& pair : *m_files)
    {
        namesArray[i++] = OUString::createFromAscii(pair.first.c_str());
    }
    return names;
}

// name is relative path to the current storage
sal_Bool SAL_CALL ExpandedStorage::hasByName(const OUString& name)
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    return m_files->find(helpers::toString(getFullPath(name))) != m_files->end();
}

css::uno::Type SAL_CALL ExpandedStorage::getElementType() { return uno::Type(); }

sal_Bool SAL_CALL ExpandedStorage::hasElements()
{
    std::lock_guard<std::mutex> lock(m_aMutex);
    return !m_files->empty();
}

// XPropertySet
css::uno::Reference<css::beans::XPropertySetInfo> SAL_CALL ExpandedStorage::getPropertySetInfo()
{
    return Reference < beans::XPropertySetInfo > ();
}

void SAL_CALL ExpandedStorage::setPropertyValue(const OUString& name, const css::uno::Any& value)
{
    m_properties.insert({name, value});
}

css::uno::Any SAL_CALL ExpandedStorage::getPropertyValue(const OUString& name)
{
    auto it = m_properties.find(name);
    if (it == m_properties.end()) {
        SAL_WARN("expandedstorage", "requested property (" << name << ") not found");
        return Any();
    }

    return Any(it->second);
}

void SAL_CALL ExpandedStorage::addPropertyChangeListener(
    const OUString&, const Reference<beans::XPropertyChangeListener>&)
{
}

void SAL_CALL ExpandedStorage::removePropertyChangeListener(
    const OUString&, const Reference<beans::XPropertyChangeListener>&)
{
}

void SAL_CALL ExpandedStorage::addVetoableChangeListener(
    const OUString&, const Reference<css::beans::XVetoableChangeListener>&)
{
}

void SAL_CALL ExpandedStorage::removeVetoableChangeListener(
    const OUString&, const Reference<beans::XVetoableChangeListener>&)
{
}

// streamPath is absolute
css::uno::Reference<css::embed::XExtendedStorageStream> SAL_CALL
ExpandedStorage::openStreamElementByHierarchicalName(const OUString& streamPath, sal_Int32 openMode)
{
    css::uno::Reference<io::XStream> xStream
        = openStreamElement(streamPath, openMode, PathType::Absolute);

    Reference<SequenceStreamContainer> aStreamContainer(new SequenceStreamContainer(xStream));
    Reference<embed::XExtendedStorageStream> xExtendedStream(aStreamContainer, UNO_QUERY_THROW);
    return xExtendedStream;
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

void SAL_CALL ExpandedStorage::addEventListener(const Reference<lang::XEventListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);

    m_aListenersContainer.addInterface(aGuard, xListener);
}

void SAL_CALL ExpandedStorage::removeEventListener(const Reference<lang::XEventListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);

    m_aListenersContainer.removeInterface(aGuard, xListener);
}

// StorageBase
bool ExpandedStorage::implIsStorage() const { return true; }

Reference<embed::XStorage> ExpandedStorage::implGetXStorage() const
{
    return Reference<embed::XStorage>(const_cast<ExpandedStorage*>(this));
}

void ExpandedStorage::implGetElementNames(::std::vector<OUString>& orElementNames) const
{
    size_t i = 0;
    for (const auto& pair : *m_files)
    {
        orElementNames[i++] = helpers::toOUString(pair.first);
    }
}

StorageRef ExpandedStorage::implOpenSubStorage(const OUString& path, bool)
{
    SAL_WARN("expandedstorage", "opening sub storage" << path);
    return std::shared_ptr<StorageBase>(
        new ExpandedStorage(m_xContext, m_files, m_inputStream, path));
}

Reference<io::XInputStream> ExpandedStorage::implOpenInputStream(const OUString& rElementName)
{
    return openStreamElement(rElementName, embed::ElementModes::READ)->getInputStream();
}

Reference<io::XOutputStream> ExpandedStorage::implOpenOutputStream(const OUString& rElementName)
{
    SAL_WARN("expandedstorage", "opening output stream" << rElementName);
    return openStreamElement(rElementName, embed::ElementModes::READWRITE)->getOutputStream();
}

void ExpandedStorage::implCommit() const {}

const beans::StringPair* lcl_findPairByName(const Sequence<beans::StringPair>& rSeq,
                                            const OUString& rName)
{
    return std::find_if(rSeq.begin(), rSeq.end(),
                        [&rName](const beans::StringPair& rPair) { return rPair.First == rName; });
}

css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>
ExpandedStorage::getRelInfoFromName(const OUString& name)
{
    uno::Reference<io::XInputStream> relInfoStream
        = openStreamElement(name, embed::ElementModes::READ, PathType::Absolute)->getInputStream();

    return ::comphelper::OFOPXMLHelper::ReadRelationsInfoSequence(relInfoStream, name, m_xContext);
}

void ExpandedStorage::readRelationshipInfo()
{
    std::vector<std::string> relFilePaths;
    // Find all files that end in .rels
    for (const auto& [path, _file] : *m_files)
    {
        if (path.find(".rels") == std::string::npos)
        {
            continue;
        }
        relFilePaths.push_back(path);
    }

    std::vector<Sequence<beans::StringPair>> allRelsVec;

    for (const std::string& path : relFilePaths)
    {
        auto seq = getRelInfoFromName(helpers::toOUString(path));

        for (const auto& rels : seq)
        {
            allRelsVec.push_back(rels);
        }
    }

    m_aRelInfo = comphelper::containerToSequence(allRelsVec);
}

void SAL_CALL ExpandedStorage::clearRelationships() { m_aRelInfo.realloc(0); }

void SAL_CALL ExpandedStorage::insertRelationships(
    const Sequence<Sequence<beans::StringPair>>& aEntries, sal_Bool bReplace)
{
    OUString aIDTag("Id");
    const Sequence<Sequence<beans::StringPair>> aSeq = getAllRelationships();
    std::vector<Sequence<beans::StringPair>> aResultVec;
    aResultVec.reserve(aSeq.getLength() + aEntries.getLength());

    std::copy_if(aSeq.begin(), aSeq.end(), std::back_inserter(aResultVec),
                 [&aIDTag, &aEntries](const Sequence<beans::StringPair>& rTargetRel)
                 {
                     auto pTargetPair = lcl_findPairByName(rTargetRel, aIDTag);
                     if (pTargetPair == rTargetRel.end())
                         return false;

                     bool bIsSourceSame = std::any_of(
                         aEntries.begin(), aEntries.end(),
                         [&pTargetPair](const Sequence<beans::StringPair>& rSourceEntry)
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
        [&aIDTag](const Sequence<beans::StringPair>& rEntry) -> Sequence<beans::StringPair>
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
    Sequence<Sequence<beans::StringPair>> aSeq = getAllRelationships();
    const beans::StringPair aIDRel("Id", sID);
    auto pRel = std::find_if(std::cbegin(aSeq), std::cend(aSeq),
                             [&aIDRel](const Sequence<beans::StringPair>& rRel)
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

Sequence<Sequence<beans::StringPair>>
    SAL_CALL ExpandedStorage::getRelationshipsByType(const OUString& sType)
{
    const Sequence<Sequence<beans::StringPair>> aSeq = getAllRelationships();
    std::vector<Sequence<beans::StringPair>> aResult;
    aResult.reserve(aSeq.getLength());

    std::copy_if(aSeq.begin(), aSeq.end(), std::back_inserter(aResult),
                 [&sType](const Sequence<beans::StringPair>& rRel)
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
    const Sequence<beans::StringPair> aSeq = getRelationshipByID(sID);
    auto pRel = lcl_findPairByName(aSeq, "Type");
    if (pRel != aSeq.end())
        return pRel->Second;

    return OUString();
}

OUString SAL_CALL ExpandedStorage::getTargetByID(const OUString& sID)
{
    const Sequence<beans::StringPair> aSeq = getRelationshipByID(sID);
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

Sequence<beans::StringPair> SAL_CALL ExpandedStorage::getRelationshipByID(const OUString& sID)
{
    const Sequence<Sequence<beans::StringPair>> aSeq = getAllRelationships();
    const beans::StringPair aIDRel("Id", sID);

    auto pRel = std::find_if(aSeq.begin(), aSeq.end(),
                             [&aIDRel](const Sequence<beans::StringPair>& rRel)
                             { return std::find(rRel.begin(), rRel.end(), aIDRel) != rRel.end(); });
    if (pRel != aSeq.end())
        return *pRel;

    throw container::NoSuchElementException();
}

Sequence<Sequence<beans::StringPair>> SAL_CALL ExpandedStorage::getAllRelationships()
{
    return m_aRelInfo;
}

} // namespace oox
