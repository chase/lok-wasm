#include <com/sun/star/embed/XRelationshipAccess.hpp>
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
#include <mutex>


namespace comphelper
{
using namespace com::sun::star::uno;
using namespace com::sun::star;

class RelationshipAccessImpl : public ::cppu::WeakImplHelper<embed::XRelationshipAccess>
{
public:
    css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>> m_aRelInfo;
    std::mutex m_mutex;

    RelationshipAccessImpl() {};

    virtual sal_Bool SAL_CALL hasByID(const OUString& sID) override;

    virtual OUString SAL_CALL getTargetByID(const OUString& sID) override;

    virtual OUString SAL_CALL getTypeByID(const OUString& sID) override;

    virtual css::uno::Sequence<css::beans::StringPair>
        SAL_CALL getRelationshipByID(const OUString& sID) override;

    virtual css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>
        SAL_CALL getRelationshipsByType(const OUString& sType) override;

    virtual css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>
        SAL_CALL getAllRelationships() override;

    virtual void SAL_CALL insertRelationshipByID(
        const OUString& sID, const css::uno::Sequence<css::beans::StringPair>& aEntry,
        sal_Bool bReplace) override;

    virtual void SAL_CALL removeRelationshipByID(const OUString& sID) override;

    virtual void SAL_CALL insertRelationships(
        const css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>>& aEntries,
        sal_Bool bReplace) override;

    virtual void SAL_CALL clearRelationships() override;
};
}
