#include "com/sun/star/uno/Sequence.h"
#include <oox/helper/expandedstorage.hxx>
namespace oox {

ExpandedStorage::ExpandedStorage()
{}

void ExpandedStorage::addPart(const std::string& path, const std::string& content)
{
    using namespace css::uno;
    OUString sPath = OUString::createFromAscii(path.c_str());
    Sequence<sal_Int8> sContent((sal_Int8*)content.c_str(), content.size());
    ExpandedFile file(sPath, sContent);

    files.insert(std::make_pair(path, file));
}

}
