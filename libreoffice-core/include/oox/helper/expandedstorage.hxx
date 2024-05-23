#include <rtl/ustring.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <string>
#include <unordered_map>


namespace com::sun::star {
    namespace embed { class XStorage; }
    namespace io { class XInputStream; }
    namespace io { class XOutputStream; }
    namespace io { class XStream; }
    namespace uno {
        class XComponentContext;
    }
}

using namespace com::sun::star;
namespace oox {


struct ExpandedFile
{
    const OUString path;
    const css::uno::Sequence<sal_Int8> content;

    ExpandedFile(const OUString& path_, const css::uno::Sequence<sal_Int8>& content_)
        : path(path_)
        , content(content_){};
};

class ExpandedStorage
{
    std::unordered_map<std::string, ExpandedFile> files;

    public:
    ExpandedStorage();

    void addPart(const std::string& path, const std::string& content);
};

}
