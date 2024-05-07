#ifndef INCLUDED_OOX_HELPER_EXPANDEDSTORAGE_HXX
#define INCLUDED_OOX_HELPER_EXPANDEDSTORAGE_HXX

#include "sal/types.h"
#include <vector>

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <oox/helper/storagebase.hxx>
#include <rtl/ustring.hxx>

namespace com::sun::star {
    namespace embed { class XStorage; }
    namespace io { class XInputStream; }
    namespace io { class XOutputStream; }
    namespace io { class XStream; }
    namespace uno {
        class XComponentContext;
    }
}

namespace oox {

using namespace com::sun::star;

struct ExpandedPart {
    const OUString&  path;
    const OUString&  sha;
    css::uno::Sequence<sal_Int8> content;

    ExpandedPart(const OUString& path_, const OUString& sha_, css::uno::Sequence<sal_Int8> content_)
        : path(path_)
        , sha(sha_)
        , content(content_){};
};

class ExpandedStorage final : public StorageBase
{
public:
    // Reading in from an existing docx file
    explicit            ExpandedStorage(
                            const css::uno::Reference< css::uno::XComponentContext >& rxContext,
                            const css::uno::Reference< css::io::XInputStream >& rxInStream,
                            bool bRepairStorage,
                            bool bFromExpanded);

    virtual             ~ExpandedStorage() override;

private:

    virtual bool        implIsStorage() const override;

    virtual css::uno::Reference< css::embed::XStorage >
                        implGetXStorage() const override;

    virtual void        implGetElementNames( ::std::vector< OUString >& orElementNames ) const override;

    virtual StorageRef  implOpenSubStorage( const OUString& rElementName, bool bCreateMissing ) override;

    virtual css::uno::Reference< css::io::XInputStream >
                        implOpenInputStream( const OUString& rElementName ) override;

    virtual css::uno::Reference< css::io::XOutputStream >
                        implOpenOutputStream( const OUString& rElementName ) override;

    virtual void        implCommit() const override;

    virtual void addPart(ExpandedPart& part);

private:
    std::unordered_map<OUString, ExpandedPart> m_parts;
};


} // namespace oox

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
