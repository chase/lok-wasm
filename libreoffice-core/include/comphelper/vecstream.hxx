#include <com/sun/star/io/XStream.hpp>
#include <config_options.h>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/io/XSeekable.hpp>
#include <cppuhelper/implbase.hxx>
#include <comphelper/comphelperdllapi.h>
#include <comphelper/bytereader.hxx>
#include <comphelper/interfacecontainer4.hxx>
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/embed/XExtendedStorageStream.hpp>
#include <com/sun/star/embed/XRelationshipAccess.hpp>
#include <comphelper/relationshipaccess.hxx>
#include <mutex>

namespace comphelper
{

using namespace com::sun::star::uno;
using namespace com::sun::star;

class COMPHELPER_DLLPUBLIC VectorInputStream final
    : public ::cppu::WeakImplHelper<io::XInputStream, io::XSeekable>,
      public comphelper::ByteReader
{
    std::vector<sal_Int8>& m_vec;
    std::mutex m_mutex;
    sal_Int32 m_pos;

public:
    VectorInputStream(std::vector<sal_Int8>& vec);
    virtual sal_Int32 SAL_CALL readBytes(Sequence<sal_Int8>& aData,
                                         sal_Int32 nBytesToRead) override;

    virtual sal_Int32 SAL_CALL readSomeBytes(Sequence<sal_Int8>& aData,
                                             sal_Int32 nMaxBytesToRead) override;

    virtual void SAL_CALL skipBytes(sal_Int32 nBytesToSkip) override;

    virtual sal_Int32 SAL_CALL available() override;

    virtual void SAL_CALL closeInput() override;

    virtual void SAL_CALL seek(sal_Int64 location) override;
    virtual sal_Int64 SAL_CALL getPosition() override;
    virtual sal_Int64 SAL_CALL getLength() override;

    virtual sal_Int32 readSomeBytes(sal_Int8* pData, sal_Int32 nBytesToRead) override;
};

class COMPHELPER_DLLPUBLIC VectorOutputStream final
    : public ::cppu::WeakImplHelper<io::XOutputStream>
{
    std::vector<sal_Int8>& m_vec;
    sal_Int32 m_pos;

    std::mutex m_mutex;

public:
    VectorOutputStream(std::vector<sal_Int8>& vec);
    virtual void SAL_CALL writeBytes(const Sequence<sal_Int8>& aData) override;
    virtual void SAL_CALL flush() override;
    virtual void SAL_CALL closeOutput() override;
};

class VecStreamSupplier final : public io::XStream,
                                public io::XSeekable,
                                public embed::XRelationshipAccess,
                                public ::cppu::OWeakObject,
                                public css::lang::XTypeProvider
{
private:
    css::uno::Reference<io::XInputStream> m_inputStream;
    css::uno::Reference<io::XOutputStream> m_outputStream;
    std::mutex m_mutex;
    css::uno::Reference<css::io::XSeekable> m_seekable;
    ::comphelper::OInterfaceContainerHelper4<css::lang::XEventListener> m_listeners;

public:
    comphelper::RelationshipAccessImpl m_relAccess;
    VecStreamSupplier(css::uno::Reference<io::XInputStream> xInput,
                      css::uno::Reference<io::XOutputStream> xOutput);

    // XStream
    virtual Reference<io::XInputStream> SAL_CALL getInputStream() override;
    virtual Reference<io::XOutputStream> SAL_CALL getOutputStream() override;

    // XSeekable
    virtual void SAL_CALL seek(sal_Int64 location) override;
    virtual sal_Int64 SAL_CALL getPosition() override;
    virtual sal_Int64 SAL_CALL getLength() override;

    /* // XRelationshipAccess */
    virtual sal_Bool SAL_CALL hasByID(const OUString& sID) override
    {
        return m_relAccess.hasByID(sID);
    }

    virtual OUString SAL_CALL getTargetByID(const OUString& sID) override
    {
        return m_relAccess.getTargetByID(sID);
    }

    virtual OUString SAL_CALL getTypeByID(const OUString& sID) override
    {
        return m_relAccess.getTypeByID(sID);
    }

    virtual css::uno::Sequence<css::beans::StringPair>
        SAL_CALL getRelationshipByID(const OUString& sID) override
    {
        return m_relAccess.getRelationshipByID(sID);
    }

    virtual css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>
        SAL_CALL getRelationshipsByType(const OUString& sType) override
    {
        return m_relAccess.getRelationshipsByType(sType);
    }

    virtual css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>
        SAL_CALL getAllRelationships() override
    {
        return m_relAccess.getAllRelationships();
    }

    virtual void SAL_CALL insertRelationshipByID(
        const OUString& sID, const css::uno::Sequence<css::beans::StringPair>& aEntry,
        sal_Bool bReplace) override
    {
        m_relAccess.insertRelationshipByID(sID, aEntry, bReplace);
    }

    virtual void SAL_CALL removeRelationshipByID(const OUString& sID) override
    {
        m_relAccess.removeRelationshipByID(sID);
    }

    virtual void SAL_CALL insertRelationships(
        const css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>& aEntries,
        sal_Bool bReplace) override
    {
        m_relAccess.insertRelationships(aEntries, bReplace);
    }

    virtual void SAL_CALL clearRelationships() override { m_relAccess.clearRelationships(); }

    // XInterface
    virtual css::uno::Any SAL_CALL queryInterface(const css::uno::Type& rType) override;
    virtual void SAL_CALL acquire() noexcept override;
    virtual void SAL_CALL release() noexcept override;

    // XTypeProvider
    virtual css::uno::Sequence<css::uno::Type> SAL_CALL getTypes() override;
    virtual css::uno::Sequence<sal_Int8> SAL_CALL getImplementationId() override;
};

class VecStreamContainer final : public embed::XExtendedStorageStream,
                                 public ::cppu::OWeakObject,
                                 public embed::XRelationshipAccess
{
    std::mutex m_mutex;
    css::uno::Reference<css::io::XStream> m_stream;
    ::comphelper::OInterfaceContainerHelper4<css::lang::XEventListener> m_listeners;

public:
    comphelper::RelationshipAccessImpl m_relAccess;
    VecStreamContainer(css::uno::Reference<css::io::XStream>& xStream);

    virtual css::uno::Any SAL_CALL queryInterface(const css::uno::Type& rType) override;
    virtual void SAL_CALL acquire() noexcept override;
    virtual void SAL_CALL release() noexcept override;

    // XComponent
    virtual void SAL_CALL dispose() override;
    virtual void SAL_CALL
    addEventListener(const css::uno::Reference<css::lang::XEventListener>& xListener) override;
    virtual void SAL_CALL
    removeEventListener(const css::uno::Reference<css::lang::XEventListener>& aListener) override;

    /* // XRelationshipAccess */
    virtual sal_Bool SAL_CALL hasByID(const OUString& sID) override
    {
        return m_relAccess.hasByID(sID);
    }

    virtual OUString SAL_CALL getTargetByID(const OUString& sID) override
    {
        return m_relAccess.getTargetByID(sID);
    }

    virtual OUString SAL_CALL getTypeByID(const OUString& sID) override
    {
        return m_relAccess.getTypeByID(sID);
    }

    virtual css::uno::Sequence<css::beans::StringPair>
        SAL_CALL getRelationshipByID(const OUString& sID) override
    {
        return m_relAccess.getRelationshipByID(sID);
    }

    virtual css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>
        SAL_CALL getRelationshipsByType(const OUString& sType) override
    {
        return m_relAccess.getRelationshipsByType(sType);
    }

    virtual css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>
        SAL_CALL getAllRelationships() override
    {
        return m_relAccess.getAllRelationships();
    }

    virtual void SAL_CALL insertRelationshipByID(
        const OUString& sID, const css::uno::Sequence<css::beans::StringPair>& aEntry,
        sal_Bool bReplace) override
    {
        m_relAccess.insertRelationshipByID(sID, aEntry, bReplace);
    }

    virtual void SAL_CALL removeRelationshipByID(const OUString& sID) override
    {
        m_relAccess.removeRelationshipByID(sID);
    }

    virtual void SAL_CALL insertRelationships(
        const css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>& aEntries,
        sal_Bool bReplace) override
    {
        m_relAccess.insertRelationships(aEntries, bReplace);
    }

    virtual void SAL_CALL clearRelationships() override { m_relAccess.clearRelationships(); }

    // XStream
    virtual css::uno::Reference<css::io::XInputStream> SAL_CALL getInputStream() override;
    virtual css::uno::Reference<css::io::XOutputStream> SAL_CALL getOutputStream() override;
};
}
