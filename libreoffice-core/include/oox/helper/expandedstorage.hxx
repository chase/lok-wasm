#ifndef INCLUDED_OOX_EXPANDEDSTORAGE_HXX
#define INCLUDED_OOX_EXPANDEDSTORAGE_HXX

#include <rtl/ustring.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <string>
#include <unordered_map>
#include <mutex>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/XHierarchicalStorageAccess.hpp>
#include <com/sun/star/io/XStream.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/lang/XTypeProvider.hpp>
#include <cppuhelper/weak.hxx>
#include <comphelper/interfacecontainer4.hxx>

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

using namespace com::sun::star;

namespace oox
{

struct ExpandedFile
{
    const OUString path;
    const css::uno::Sequence<sal_Int8> content;

    ExpandedFile(const OUString& path_, const css::uno::Sequence<sal_Int8>& content_)
        : path(path_)
        , content(content_){};
};

class ExpandedStorage final : public css::lang::XTypeProvider,
                              public css::embed::XStorage,
                              public css::embed::XHierarchicalStorageAccess,
                              public css::beans::XPropertySet,
                              public cppu::OWeakObject
{
    std::unordered_map<std::string, ExpandedFile> files;
    std::mutex m_aMutex;
    css::uno::Reference<css::uno::XComponentContext> m_xContext;

public:
    ExpandedStorage();

    void addPart(const std::string& path, const std::string& content);

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

    void disposeImpl(std::unique_lock<std::mutex>& rGuard);
};

} // namespace oox

#endif // INCLUDED_OOX_EXPANDEDSTORAGE_HXX
