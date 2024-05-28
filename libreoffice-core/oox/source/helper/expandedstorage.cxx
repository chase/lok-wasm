#include "com/sun/star/io/SequenceInputStream.hpp"
#include "com/sun/star/io/XStream.hdl"
#include "com/sun/star/packages/NoEncryptionException.hdl"
#include "com/sun/star/uno/Reference.h"
#include "com/sun/star/uno/Sequence.h"
#include "oox/helper/storagebase.hxx"
#include "sot/stg.hxx"
#include "tools/stream.hxx"
#include "unotools/streamwrap.hxx"
#include <memory>
#include <oox/helper/expandedstorage.hxx>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/InvalidStorageException.hpp>
#include <com/sun/star/embed/StorageWrappedTargetException.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/io/XTruncate.hpp>
#include <comphelper/processfactory.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <osl/diagnose.h>

using namespace com::sun::star;

namespace oox
{

ExpandedStorage::ExpandedStorage(const css::uno::Reference< uno::XComponentContext >& rxContext, const css::uno::Reference< css::io::XInputStream >& rxInStream) :
    StorageBase(rxInStream, false),
    m_xContext(rxContext)
{}

void ExpandedStorage::addPart(const std::string& path, const std::string& content)
{
    using namespace css::uno;
    OUString sPath = OUString::createFromAscii(path.c_str());
    Sequence<sal_Int8> sContent((sal_Int8*)content.c_str(), content.size());
    ExpandedFile file(sPath, sContent);
    files.insert({ path, file });
}

uno::Reference<StorageBase> ExpandedStorage::getStorageBase()
{
    return uno::Reference<StorageBase>(this);
}

// XInterface
css::uno::Any SAL_CALL ExpandedStorage::queryInterface(const css::uno::Type& rType)
{
    uno::Any aReturn = ::cppu::queryInterface(
        rType, static_cast<lang::XTypeProvider*>(this), static_cast<embed::XStorage*>(this),
        static_cast<embed::XHierarchicalStorageAccess*>(this),
        static_cast<container::XNameAccess*>(this), static_cast<container::XElementAccess*>(this),
        static_cast<lang::XComponent*>(this), static_cast<beans::XPropertySet*>(this));

    if (aReturn.hasValue())
        return aReturn;

    return OWeakObject::queryInterface(rType);
}

void SAL_CALL ExpandedStorage::acquire() noexcept { cppu::OWeakObject::acquire(); }

void SAL_CALL ExpandedStorage::release() noexcept { cppu::OWeakObject::release(); }

// XTypeProvider
css::uno::Sequence<css::uno::Type> SAL_CALL ExpandedStorage::getTypes()
{
    static css::uno::Sequence<css::uno::Type> aTypes
        = { cppu::UnoType<css::lang::XTypeProvider>::get(),
            cppu::UnoType<css::embed::XStorage>::get(),
            cppu::UnoType<css::embed::XHierarchicalStorageAccess>::get(),
            cppu::UnoType<css::beans::XPropertySet>::get()};
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

    SvMemoryStream aMemStream;
    aMemStream.WriteBytes(file.content.getConstArray(), file.content.getLength());
    aMemStream.Seek(STREAM_SEEK_TO_BEGIN);

    uno::Reference<io::XStream> xStream = new utl::OStreamWrapper(aMemStream);
    return css::uno::Reference<css::io::XStream>(xStream);
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
bool ExpandedStorage::implIsStorage() const
{
    return true;
}

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


} // namespace oox
