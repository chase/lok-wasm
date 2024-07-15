#ifndef INCLUDED_OOX_EXPANDEDSTORAGE_HXX
#define INCLUDED_OOX_EXPANDEDSTORAGE_HXX
#include <memory>
#include <sal/types.h>
#include <com/sun/star/embed/XExtendedStorageStream.hpp>
#include <boost/unordered/unordered_map_fwd.hpp>
#include <com/sun/star/embed/XExtendedStorageStream.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XSeekable.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <comphelper/seqstream.hxx>
#include <cppuhelper/implbase.hxx>
#include <oox/helper/storagebase.hxx>
#include <rtl/ustring.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <string>
#include <unordered_map>
#include <boost/unordered_map.hpp>
#include <mutex>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/XHierarchicalStorageAccess.hpp>
#include <com/sun/star/embed/XRelationshipAccess.hpp>
#include <com/sun/star/io/XStream.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/lang/XTypeProvider.hpp>
#include <cppuhelper/weak.hxx>
#include <comphelper/interfacecontainer4.hxx>
#include <comphelper/relationshipaccess.hxx>
#include <unordered_map>

namespace com::sun::star
{
namespace embed
{
class XStorage;
}
namespace io
{
class XInputStream;
}
namespace io
{
class XOutputStream;
}
namespace io
{
class XStream;
}
namespace uno
{
class XComponentContext;
}
}

namespace comphelper
{
class VecStreamSupplier;
}

using namespace com::sun::star;

namespace oox
{

typedef std::vector<unsigned char> ShaVec;

struct ExpandedFile
{
    const OUString path;
    std::shared_ptr<ShaVec> sha;
    std::shared_ptr<std::vector<sal_Int8>> content;
    // Track the number of opened output streams for the content in the file;
    sal_Int32 writeRefCount;

    ExpandedFile(const OUString& path_, const std::vector<sal_Int8>&& content_, const ShaVec&& sha_)
        : path(path_)
        , sha(std::make_shared<ShaVec>(std::move(sha_)))
        , content(std::make_shared<std::vector<sal_Int8>>(std::move(content_))) {};
};

typedef boost::unordered_map<std::string, ExpandedFile> ExpandedFileMap;
enum PathType
{
    Relative,
    Absolute
};

struct Commit
{
    std::vector<std::pair<std::string, std::string>> filesChanged;
    std::time_t timestamp;

    Commit(std::vector<std::pair<std::string, std::string>> filesChanged_, std::time_t timestamp_)
        : filesChanged(filesChanged_)
        , timestamp(timestamp_)
    {
    }
};

class ExpandedStorage final : public css::lang::XTypeProvider,
                              public css::embed::XStorage,
                              public css::embed::XHierarchicalStorageAccess,
                              public css::embed::XRelationshipAccess,
                              public css::beans::XPropertySet,
                              public cppu::OWeakObject,
                              public StorageBase
{
    std::shared_ptr<comphelper::RelationshipAccessImpl> m_relAccess;
    std::shared_ptr<
        boost::unordered_map<std::string, std::shared_ptr<comphelper::RelationshipAccessImpl>>>
        m_allRelAccessMap;
    std::shared_ptr<ExpandedFileMap> m_files;
    std::shared_ptr<Commit> m_lastCommit;
    std::vector<OUString> m_dirs;
    std::mutex m_aMutex;
    css::uno::Reference<css::uno::XComponentContext> m_xContext;
    ::comphelper::OInterfaceContainerHelper4<css::lang::XEventListener> m_aListenersContainer;
    std::optional<OUString> m_basePath;
    css::uno::Reference<css::io::XInputStream> m_inputStream;
    std::unordered_map<OUString, css::uno::Any> m_properties;

public:
    ExpandedStorage(const css::uno::Reference<css::uno::XComponentContext>& rxContext,
                    const css::uno::Reference<css::io::XInputStream>& rxStream);
    // Constructor for creating a sub storage
    ExpandedStorage(
        const css::uno::Reference<css::uno::XComponentContext>& rxContext,
        const std::shared_ptr<ExpandedFileMap>& fileMap,
        const css::uno::Reference<io::XInputStream>& rxInputStream, const OUString& basePath,
        std::shared_ptr<
            boost::unordered_map<std::string, std::shared_ptr<comphelper::RelationshipAccessImpl>>>
            allRelAccessMap,
        std::shared_ptr<Commit> lastCommit);

    ExpandedStorage(const ExpandedStorage&) = delete;
    ExpandedStorage(ExpandedStorage&&) = delete;
    ExpandedStorage& operator=(const ExpandedStorage&) = delete;
    ExpandedStorage& operator=(ExpandedStorage&&) = delete;

    void addPart(const std::string& path, const std::string& content);

    std::optional<std::pair<std::string, std::shared_ptr<std::vector<sal_Int8>>>>
    getPart(const std::string& path) const;

    void removePart(const std::string& path);
    std::vector<std::pair<const std::string, const std::string>> listParts();

    void afterCommit();

    void commitRelationships();

    std::vector<std::pair<std::string, std::string>> getRecentlyChangedFiles();

    void disposeImpl(std::unique_lock<std::mutex>& rGuard);

    std::optional<comphelper::RelInfoSeq> getRelInfoForElement(const std::string& path);
    css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>
    getRelInfoFromName(const OUString& name);
    void readRelationshipInfo();
    OUString getFullPath(const OUString& path) const;
    uno::Reference<io::XStream> openStreamElement(const OUString& name, sal_Int32 openMode,
                                                  PathType pathType, bool readRelInfo = true);

    uno::Reference<comphelper::VecStreamSupplier>
    openStreamElementSupplier(const OUString& name, sal_Int32 openMode, PathType pathType,
                              bool readRelInfo = true);

    // XInterface
    virtual css::uno::Any SAL_CALL queryInterface(const css::uno::Type& rType) override;
    virtual void SAL_CALL acquire() noexcept override;
    virtual void SAL_CALL release() noexcept override;

    // XTypeProvider
    virtual css::uno::Sequence<css::uno::Type> SAL_CALL getTypes() override;
    virtual css::uno::Sequence<sal_Int8> SAL_CALL getImplementationId() override;

    // XStorage
    virtual void SAL_CALL
    copyToStorage(const css::uno::Reference<css::embed::XStorage>& xDest) override;
    virtual css::uno::Reference<css::io::XStream>
        SAL_CALL openStreamElement(const OUString& aStreamName, sal_Int32 nOpenMode) override;
    virtual css::uno::Reference<css::io::XStream>
        SAL_CALL openEncryptedStreamElement(const OUString& aStreamName, sal_Int32 nOpenMode,
                                            const OUString& aPass) override;
    virtual css::uno::Reference<css::embed::XStorage>
        SAL_CALL openStorageElement(const OUString& aStorName, sal_Int32 nStorageMode) override;
    virtual css::uno::Reference<css::io::XStream>
        SAL_CALL cloneStreamElement(const OUString& aStreamName) override;
    virtual css::uno::Reference<css::io::XStream> SAL_CALL
    cloneEncryptedStreamElement(const OUString& aStreamName, const OUString& aPass) override;
    virtual void SAL_CALL
    copyLastCommitTo(const css::uno::Reference<css::embed::XStorage>& xTargetStorage) override;
    virtual void SAL_CALL copyStorageElementLastCommitTo(
        const OUString& aStorName,
        const css::uno::Reference<css::embed::XStorage>& xTargetStorage) override;
    virtual sal_Bool SAL_CALL isStreamElement(const OUString& aElementName) override;
    virtual sal_Bool SAL_CALL isStorageElement(const OUString& aElementName) override;
    virtual void SAL_CALL removeElement(const OUString& aElementName) override;
    virtual void SAL_CALL renameElement(const OUString& rEleName,
                                        const OUString& rNewName) override;
    virtual void SAL_CALL copyElementTo(const OUString& aElementName,
                                        const css::uno::Reference<css::embed::XStorage>& xDest,
                                        const OUString& aNewName) override;
    virtual void SAL_CALL moveElementTo(const OUString& aElementName,
                                        const css::uno::Reference<css::embed::XStorage>& xDest,
                                        const OUString& rNewName) override;

    // XNameAccess
    virtual css::uno::Any SAL_CALL getByName(const OUString& aName) override;
    virtual css::uno::Sequence<OUString> SAL_CALL getElementNames() override;
    virtual sal_Bool SAL_CALL hasByName(const OUString& aName) override;
    virtual css::uno::Type SAL_CALL getElementType() override;
    virtual sal_Bool SAL_CALL hasElements() override;

    // XPropertySet
    virtual css::uno::Reference<css::beans::XPropertySetInfo>
        SAL_CALL getPropertySetInfo() override;
    virtual void SAL_CALL setPropertyValue(const OUString& aPropertyName,
                                           const css::uno::Any& aValue) override;
    virtual css::uno::Any SAL_CALL getPropertyValue(const OUString& PropertyName) override;
    virtual void SAL_CALL addPropertyChangeListener(
        const OUString& aPropertyName,
        const css::uno::Reference<css::beans::XPropertyChangeListener>& xListener) override;
    virtual void SAL_CALL removePropertyChangeListener(
        const OUString& aPropertyName,
        const css::uno::Reference<css::beans::XPropertyChangeListener>& aListener) override;
    virtual void SAL_CALL addVetoableChangeListener(
        const OUString& PropertyName,
        const css::uno::Reference<css::beans::XVetoableChangeListener>& aListener) override;
    virtual void SAL_CALL removeVetoableChangeListener(
        const OUString& PropertyName,
        const css::uno::Reference<css::beans::XVetoableChangeListener>& aListener) override;

    // XHierarchicalStorageAccess
    virtual css::uno::Reference<css::embed::XExtendedStorageStream> SAL_CALL
    openStreamElementByHierarchicalName(const OUString& sStreamPath, sal_Int32 nOpenMode) override;
    virtual css::uno::Reference<css::embed::XExtendedStorageStream>
        SAL_CALL openEncryptedStreamElementByHierarchicalName(const OUString& sStreamName,
                                                              sal_Int32 nOpenMode,
                                                              const OUString& sPassword) override;
    virtual void SAL_CALL
    removeStreamElementByHierarchicalName(const OUString& sElementPath) override;

    //  XComponent

    virtual void SAL_CALL dispose() override;

    virtual void SAL_CALL
    addEventListener(const css::uno::Reference<css::lang::XEventListener>& xListener) override;

    virtual void SAL_CALL
    removeEventListener(const css::uno::Reference<css::lang::XEventListener>& xListener) override;

    // StorageBase
    /** Returns true, if the object represents a valid storage. */
    virtual bool implIsStorage() const override;

    /** Returns the com.sun.star.embed.XStorage interface of the current storage. */
    virtual css::uno::Reference<css::embed::XStorage> implGetXStorage() const override;

    /** Returns the names of all elements of this storage. */
    virtual void implGetElementNames(::std::vector<OUString>& orElementNames) const override;

    /** Opens and returns the specified sub storage from the storage. */
    virtual StorageRef implOpenSubStorage(const OUString& rElementName,
                                          bool bCreateMissing) override;

    /** Opens and returns the specified input stream from the storage. */
    virtual css::uno::Reference<css::io::XInputStream>
    implOpenInputStream(const OUString& rElementName) override;

    /** Opens and returns the specified output stream from the storage. */
    virtual css::uno::Reference<css::io::XOutputStream>
    implOpenOutputStream(const OUString& rElementName) override;

    virtual css::uno::Reference<css::io::XInputStream>
    openInputStream(const OUString& rStreamName) override;

    virtual css::uno::Reference<css::io::XOutputStream>
    openOutputStream(const OUString& rStreamName) override;

    /** Commits the current storage. */
    virtual void implCommit() const override;

    /* // XRelationshipAccess */
    virtual sal_Bool SAL_CALL hasByID(const OUString& sID) override
    {
        return m_relAccess->hasByID(sID);
    }

    virtual OUString SAL_CALL getTargetByID(const OUString& sID) override
    {
        return m_relAccess->getTargetByID(sID);
    }

    virtual OUString SAL_CALL getTypeByID(const OUString& sID) override
    {
        return m_relAccess->getTypeByID(sID);
    }

    virtual css::uno::Sequence<css::beans::StringPair>
        SAL_CALL getRelationshipByID(const OUString& sID) override
    {
        return m_relAccess->getRelationshipByID(sID);
    }

    virtual css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>
        SAL_CALL getRelationshipsByType(const OUString& sType) override
    {
        return m_relAccess->getRelationshipsByType(sType);
    }

    virtual css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>
        SAL_CALL getAllRelationships() override
    {
        return m_relAccess->getAllRelationships();
    }

    virtual void SAL_CALL insertRelationshipByID(
        const OUString& sID, const css::uno::Sequence<css::beans::StringPair>& aEntry,
        sal_Bool bReplace) override
    {
        m_relAccess->insertRelationshipByID(sID, aEntry, bReplace);
    }

    virtual void SAL_CALL removeRelationshipByID(const OUString& sID) override
    {
        m_relAccess->removeRelationshipByID(sID);
    }

    virtual void SAL_CALL insertRelationships(
        const css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>& aEntries,
        sal_Bool bReplace) override
    {
        m_relAccess->insertRelationships(aEntries, bReplace);
    }

    virtual void SAL_CALL clearRelationships() override { m_relAccess->clearRelationships(); }
};

} // namespace oox

#endif // INCLUDED_OOX_EXPANDEDSTORAGE_HXX
