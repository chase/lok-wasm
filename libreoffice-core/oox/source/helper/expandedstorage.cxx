#include <comphelper/relationshipaccess.hxx>
#include <com/sun/star/beans/UnknownPropertyException.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/XExtendedStorageStream.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XStream.hpp>
#include <com/sun/star/packages/NoEncryptionException.hpp>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/uno/Sequence.h>
#include <comphelper/diagnose_ex.hxx>
#include <comphelper/hash.hxx>
#include <comphelper/vecstream.hxx>
#include <comphelper/sequence.hxx>
#include <oox/helper/binaryinputstream.hxx>
#include <oox/helper/binaryoutputstream.hxx>
#include <oox/helper/storagebase.hxx>
#include <osl/thread.h>
#include <sal/log.hxx>
#include <sot/stg.hxx>
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
inline constexpr char REL_DIR_NAME[] = "/_rels/";
inline constexpr char REL_EXT[] = ".rels";

namespace helpers
{


std::string toString(const OUString& value) { return std::string(value.toUtf8()); }

// https://github.com/chase/awrit/blob/546772897461f7a4654d30450547f95f11c0f6b6/awrit/string/string_utils.cc#L11-L26
std::vector<std::string_view> split(const std::string_view& str, char delimiter)
{
    if (str.empty())
        return {};

    std::vector<std::string_view> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);
    while (end != std::string_view::npos)
    {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(delimiter, start);
    }
    tokens.push_back(str.substr(start));

    return tokens;
}

std::string shaVecToString(const std::vector<unsigned char>& a)
{
    std::stringstream stringStream;
    for (auto& i : a)
    {
        stringStream << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(i);
    }

    return stringStream.str();
}

ShaVec getContentHash(const std::vector<sal_Int8>& content)
{
    const unsigned char* unsignedData = reinterpret_cast<const unsigned char*>(content.data());

    // Generate SHA-256 hash
    std::vector<unsigned char> hashVec = comphelper::Hash::calculateHash(
        unsignedData, content.size(), comphelper::HashType::SHA256);

    return hashVec;
}
}

ExpandedStorage::ExpandedStorage(const Reference<XComponentContext>& rxContext,
                                 const Reference<io::XInputStream>& rxInStream)
    : StorageBase(rxInStream, false, false)
    , m_lastCommit(std::shared_ptr<Commit>())
    , m_xContext(rxContext)
    , m_inputStream(rxInStream)
{}

OUString ExpandedStorage::getFullPath(const OUString& path) const
{
    return m_basePath.value_or("") + "/" + path;
}

ExpandedStorage::ExpandedStorage(
    const Reference<XComponentContext>& context_, const std::shared_ptr<ExpandedFileMap>& fileMap_,
    const Reference<io::XInputStream>& inputStream_, const OUString& basePath_,
    css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>> aRelInfo_,
    std::shared_ptr<Commit> commitLog_)
    : StorageBase(inputStream_, false, false)
    , m_files(fileMap_)
    , m_lastCommit(commitLog_)
    , m_xContext(context_)
    , m_basePath(basePath_)
    , m_inputStream(inputStream_)
{
    readRelationshipInfo();
}

void ExpandedStorage::addPart(const std::string& path, const std::string& content)
{
    std::vector<std::string_view> pathParts = helpers::split(path, '/');
    for (int idx = 0; idx < pathParts.size() - 1; idx++)
    {
        bool found = std::find(m_dirs.begin(), m_dirs.end(), OUString::fromUtf8(pathParts[idx]))
                     != m_dirs.end();
        if (!found)
        {
            m_dirs.push_back(OUString::fromUtf8(pathParts[idx]));
        }
    }
    OUString sPath = OUString::createFromAscii(path.c_str());
    std::vector<sal_Int8> fileContent(content.begin(), content.end());
    ShaVec sha = helpers::getContentHash(fileContent);

    m_files->insert({ path, { sPath, fileContent, sha } });
}

std::optional<std::pair<std::string, std::vector<sal_Int8>>>
ExpandedStorage::getPart(const std::string& path) const
{
    auto it = std::find_if(m_files->begin(), m_files->end(),
                           [&path](const auto& pair) { return pair.first == path; });

    if (it == m_files->end())
    {
        SAL_WARN("expandedstorage", "getPart: part not found " << path);
        return {};
    }

    return std::make_pair(helpers::toString(it->second.path), it->second.content);
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
    std::vector<std::pair<const std::string, const std::string>> parts;
    for (const auto& [path, file] : *m_files)
    {
        const std::string pathString = helpers::toString(file.path);
        const std::string shaString = helpers::shaVecToString(file.sha);

        parts.push_back({ pathString, shaString });
    }

    if (parts.empty())
    {
        SAL_WARN("expandedstorage", "listParts: no parts found");
    }

    return parts;
}

std::vector<std::pair<std::string, std::string>> ExpandedStorage::getRecentlyChangedFiles()
{
    if (!m_lastCommit || m_lastCommit->filesChanged.empty())
    {
        return {};
    }

    return m_lastCommit->filesChanged;
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
    return (openMode != embed::ElementModes::READ) && (openMode != embed::ElementModes::NOCREATE)
           && (openMode != embed::ElementModes::SEEKABLEREAD);
}

// TODO: @synoet
// There should be a more efficient way to load relations per file
// Maybe moving back to the original approach of loading all relations then copying over
// as needed instead of per element / storage
std::optional<comphelper::RelInfoSeq> ExpandedStorage::getRelInfoForElement(const std::string& path)
{
    std::string base = path.substr(0, path.find_last_of('/'));
    std::string name = path.substr(path.find_last_of('/') + 1);
    std::string relInfoPath = base + REL_DIR_NAME + name + REL_EXT;

    if (!m_files->contains(relInfoPath))
    {
        return {};
    }

    return getRelInfoFromName(OUString::fromUtf8(relInfoPath));
}

Reference<io::XStream> ExpandedStorage::openStreamElement(const OUString& name, sal_Int32 nOpenMode,
                                                          PathType pathType, bool readRelInfo)
{
    std::string path = pathType == PathType::Absolute ? helpers::toString(name)
                                                      : helpers::toString(getFullPath(name));

    auto it = m_files->find(std::string(path));
    if (it == m_files->end())
    {
        if (!shouldCreateStreamElement(nOpenMode))
        {
            throw css::container::NoSuchElementException();
        }

        it = m_files
                 ->insert({ path,
                            { OUString::fromUtf8(path), std::vector<sal_Int8>(),
                              std::vector<unsigned char>() } })
                 .first;
    }

    auto& file = it->second;

    Reference<io::XInputStream> maybeInputStream;
    Reference<io::XOutputStream> maybeOutputStream;

    if (nOpenMode == embed::ElementModes::READWRITE || nOpenMode == embed::ElementModes::READ
        || nOpenMode == embed::ElementModes::SEEKABLE
        || nOpenMode == embed::ElementModes::SEEKABLEREAD)
    {
        Reference<io::XInputStream> inputStream(new comphelper::VectorInputStream(file.content));
        maybeInputStream = inputStream;
    }

    if (nOpenMode == embed::ElementModes::READWRITE || nOpenMode == embed::ElementModes::WRITE
        || nOpenMode == embed::ElementModes::TRUNCATE)
    {
        Reference<io::XOutputStream> outputStream(new comphelper::VectorOutputStream(file.content));
        maybeOutputStream = outputStream;
    }

    Reference<comphelper::VecStreamSupplier> streamSupplier(
        new comphelper::VecStreamSupplier(maybeInputStream, maybeOutputStream));

    if (readRelInfo)
    {
        auto relInfo = getRelInfoForElement(path);
        if (relInfo.has_value())
        {
            streamSupplier->m_relAccess.m_aRelInfo = relInfo.value();
        }
    }

    return Reference<io::XStream>(streamSupplier);
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

    OUString base = m_basePath.has_value() ? m_basePath.value() + "/" : OUString("");
    OUString newPath = base + path;
    Reference<ExpandedStorage> expandedStorage(
        new ExpandedStorage(m_xContext, m_files, m_inputStream, newPath, m_relAccess.m_aRelInfo, m_lastCommit));
    return Reference<embed::XStorage>(expandedStorage);
}

Reference<io::XStream> SAL_CALL ExpandedStorage::cloneStreamElement(const OUString& path)
{
    auto it = m_files->find(helpers::toString(path));

    // copy the content of the original file
    auto content = std::vector<sal_Int8>(it->second.content);
    Reference<io::XInputStream> inputStream(new comphelper::VectorInputStream(content));
    Reference<io::XOutputStream> outputStream(new comphelper::VectorOutputStream(content));

    return Reference<io::XStream>(
        new comphelper::VecStreamSupplier(std::move(inputStream), std::move(outputStream)));
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
    return std::find(m_dirs.begin(), m_dirs.end(), path) != m_dirs.end();
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
    return Reference<beans::XPropertySetInfo>();
}

void SAL_CALL ExpandedStorage::setPropertyValue(const OUString& name, const css::uno::Any& value)
{
    m_properties.insert({ name, value });
}

css::uno::Any SAL_CALL ExpandedStorage::getPropertyValue(const OUString& name)
{
    auto it = m_properties.find(name);
    if (it == m_properties.end())
    {
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

css::uno::Reference<css::embed::XExtendedStorageStream> SAL_CALL
ExpandedStorage::openStreamElementByHierarchicalName(const OUString& streamPath, sal_Int32 openMode)
{
    css::uno::Reference<io::XStream> xStream
        = openStreamElement(streamPath, openMode, PathType::Absolute, true);

    Reference<comphelper::VecStreamContainer> aStreamContainer(
        new comphelper::VecStreamContainer(xStream));

    // Copy over the relationship info
    auto relInfo = getRelInfoForElement(helpers::toString(streamPath));
    if (relInfo.has_value())
        aStreamContainer->m_relAccess.m_aRelInfo = relInfo.value();

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
        orElementNames[i++] = OUString::fromUtf8(pair.first);
    }
}

StorageRef ExpandedStorage::implOpenSubStorage(const OUString& path, bool bCreateMissing)
{
    if (!bCreateMissing)
    {
        if (std::find(m_dirs.begin(), m_dirs.end(), path) == m_dirs.end() || path == "_rels")
        {
            throw css::uno::RuntimeException();
        }
    }
    return std::shared_ptr<StorageBase>(
        new ExpandedStorage(m_xContext, m_files, m_inputStream, path, m_relAccess.m_aRelInfo, m_lastCommit));
}

Reference<io::XInputStream> ExpandedStorage::implOpenInputStream(const OUString& rElementName)
{
    SAL_WARN("expandedstorage", "openInputStream" << rElementName);
    return openStreamElement(rElementName, embed::ElementModes::READ)->getInputStream();
}

Reference<io::XOutputStream> ExpandedStorage::implOpenOutputStream(const OUString& rElementName)
{
    return openStreamElement(rElementName, embed::ElementModes::READWRITE)->getOutputStream();
}

void ExpandedStorage::implCommit() const
{
    // implCommit is defined as const in StorageBase
    // we don't have this limitation and need to call afterCommit
    // which modifies the sha of the files after changes
    const_cast<ExpandedStorage*>(this)->afterCommit();
}

void ExpandedStorage::afterCommit()
{
    std::vector<std::pair<std::string, std::string>> filesChanged;
    for (auto& [path, file] : *m_files)
    {
        auto newSha = helpers::getContentHash(file.content);

        if (newSha != file.sha)
        {
            file.sha = newSha;
            std::string shaString = helpers::shaVecToString(newSha);
            filesChanged.push_back({ path, shaString });
        }
    }

    auto ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    m_lastCommit.reset(new Commit(std::move(filesChanged), ts));
}

const beans::StringPair* lcl_findPairByName(const Sequence<beans::StringPair>& rSeq,
                                            const OUString& rName)
{
    return std::find_if(rSeq.begin(), rSeq.end(),
                        [&rName](const beans::StringPair& rPair) { return rPair.First == rName; });
}

css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>
ExpandedStorage::getRelInfoFromName(const OUString& name)
{
    uno::Reference<io::XInputStream> relInfoStream;
    relInfoStream = openStreamElement(name, embed::ElementModes::READ, PathType::Absolute, false)
                        ->getInputStream();

    return ::comphelper::OFOPXMLHelper::ReadRelationsInfoSequence(relInfoStream, name, m_xContext);
}

void ExpandedStorage::readRelationshipInfo()
{
    std::vector<std::string> relFilePaths;
    std::string suffix
        = (m_basePath.has_value() ? helpers::toString(m_basePath.value()) + "/" : "") + "_rels";
    for (const auto& [path, _file] : *m_files)
    {
        if (!path.starts_with(suffix) || path.find(REL_EXT) == std::string::npos)
        {
            continue;
        }
        relFilePaths.push_back(path);
    }

    std::vector<Sequence<beans::StringPair>> allRelsVec;

    for (const std::string& path : relFilePaths)
    {
        auto seq = getRelInfoFromName(OUString::fromUtf8(path));

        for (const auto& rels : seq)
        {
            allRelsVec.push_back(rels);
        }
    }

    m_relAccess.m_aRelInfo = comphelper::containerToSequence(allRelsVec);
}

css::uno::Reference<css::io::XInputStream> ExpandedStorage::openInputStream(const OUString &rStreamName)
{
    return openStreamElementByHierarchicalName(rStreamName, embed::ElementModes::READ)->getInputStream();
}

} // namespace oox
