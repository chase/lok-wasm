#include <comphelper/ofopxmlhelper.hxx>
#include <vector>
#include <comphelper/processfactory.hxx>
#include <comphelper/relationshipaccess.hxx>
#include <comphelper/vecstream.hxx>
#include <com/sun/star/container/NoSuchElementException.hpp>
#include <com/sun/star/embed/InvalidStorageException.hpp>
#include <com/sun/star/embed/StorageWrappedTargetException.hpp>
#include <com/sun/star/packages/NoEncryptionException.hpp>
#include <comphelper/diagnose_ex.hxx>
#include <comphelper/sequence.hxx>
#include <com/sun/star/io/BufferSizeExceededException.hpp>
#include <com/sun/star/io/NotConnectedException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XEventListener.hpp>
#include <sal/types.h>

namespace comphelper
{
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::uno;
using namespace ::osl;

const beans::StringPair* lcl_findPairByName(const Sequence<beans::StringPair>& rSeq,
                                            const OUString& rName)
{
    return std::find_if(rSeq.begin(), rSeq.end(),
                        [&rName](const beans::StringPair& rPair) { return rPair.First == rName; });
}

void RelationshipAccessImpl::setRelationships(
    css::uno::Sequence<css::uno::Sequence<css::beans::StringPair>> aRelInfo)
{
    m_aRelInfo = std::move(aRelInfo);
}

void SAL_CALL RelationshipAccessImpl::clearRelationships() { m_aRelInfo.realloc(0); }

void SAL_CALL RelationshipAccessImpl::insertRelationships(
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

void SAL_CALL RelationshipAccessImpl::removeRelationshipByID(const OUString& sID)
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

        m_aRelInfo = std::move(aSeq);

        // TODO/LATER: in future the unification of the ID could be checked
        return;
    }

    throw container::NoSuchElementException();
}
void SAL_CALL RelationshipAccessImpl::insertRelationshipByID(
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
    SAL_CALL RelationshipAccessImpl::getRelationshipsByType(const OUString& sType)
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

OUString SAL_CALL RelationshipAccessImpl::getTypeByID(const OUString& sID)
{
    const Sequence<beans::StringPair> aSeq = getRelationshipByID(sID);
    auto pRel = lcl_findPairByName(aSeq, "Type");
    if (pRel != aSeq.end())
        return pRel->Second;

    return OUString();
}

OUString SAL_CALL RelationshipAccessImpl::getTargetByID(const OUString& sID)
{
    const Sequence<beans::StringPair> aSeq = getRelationshipByID(sID);
    auto pRel = lcl_findPairByName(aSeq, "Target");

    if (pRel != aSeq.end())
        return pRel->Second;

    return OUString();
}

sal_Bool SAL_CALL RelationshipAccessImpl::hasByID(const OUString& sID)
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

Sequence<beans::StringPair>
    SAL_CALL RelationshipAccessImpl::getRelationshipByID(const OUString& sID)
{
    const Sequence<Sequence<beans::StringPair>> aSeq = getAllRelationships();
    const beans::StringPair aIDRel("Id", sID);

    auto pRel = std::find_if(aSeq.begin(), aSeq.end(),
                             [&aIDRel](const Sequence<beans::StringPair>& rRel)
                             {
                                 auto id = lcl_findPairByName(rRel, "Id");
                                 return id != rRel.end() && id->Second == aIDRel.Second;
                             });
    if (pRel != aSeq.end())
        return *pRel;

    throw container::NoSuchElementException();
}

Sequence<Sequence<beans::StringPair>> SAL_CALL RelationshipAccessImpl::getAllRelationships()
{
    return m_aRelInfo;
}
}
