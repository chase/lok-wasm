#include <algorithm>
#include <docufld.hxx>
#include <tools/long.hxx>
#include <postithelper.hxx>
#include <AnnotationWin.hxx>
#include <PostItMgr.hxx>
#include <unotools/datetime.hxx>

#include <utility>
#include <view.hxx>
#include <doc.hxx>
#include <docsh.hxx>
#include <wdocsh.hxx>
#include <wrtsh.hxx>
#include <pview.hxx>
#include <viewsh.hxx>
#include <unotxdoc.hxx>
#include <tools/gen.hxx>
#include <swrect.hxx>
#include <rtl/ustring.hxx>
#include <comphelper/dispatchcommand.hxx>
#include <comphelper/propertysequence.hxx>
#include <frame.hxx>
#include <rootfrm.hxx>
#include <svl/style.hxx>
#include <SwSearchCancel.hxx>
#include <unosrch.hxx>
#include <IDocumentUndoRedo.hxx>
#include <IDocumentLayoutAccess.hxx>
#include <com/sun/star/util/XSearchDescriptor.hpp>
#include <memory>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/text/XWordCursor.hpp>
#include <com/sun/star/text/XTextViewCursorSupplier.hpp>
#include <com/sun/star/text/XTextViewCursor.hpp>
#include <com/sun/star/text/XTextRangeCompare.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/beans/XMultiPropertySet.hpp>
#include <emscripten/console.h>
#include <rtl/ref.hxx>
#include <unotextrange.hxx>
#include <SwRewriter.hxx>
#include <UndoInsert.hxx>
#include <ndarr.hxx>
#include <IDocumentOutlineNodes.hxx>
#include <ndtxt.hxx>
#include <txtfrm.hxx>

using namespace emscripten;

namespace
{
// based on RectangleTemplateBase::toString
val rectToArray(const tools::Rectangle& rect)
{
    val res = val::array();
    res.call<void>("push", rect.Left(), rect.Top(), rect.getOpenWidth(), rect.getOpenHeight());
    return res;
}

val rectToArray(const SwRect& rect) { return rectToArray(rect.SVRect()); }

val swRectsToArray(SwRects* pRects)
{
    val rArray = val::array();
    for (const SwRect& rNextRect : *pRects)
    {
        rArray.call<void>("push", rectToArray(rNextRect));
    }
    return rArray;
}

tools::Long bottomTwips(SwRects* pRects)
{
    tools::Long r = 0;
    for (const SwRect& rNextRect : *pRects)
    {
        if (rNextRect.Bottom() > r)
            r = rNextRect.Bottom();
    }
    return r;
}

tools::Long topTwips(SwRects* pRects)
{
    tools::Long r = LONG_MAX;
    for (const SwRect& rNextRect : *pRects)
    {
        if (rNextRect.Top() < r)
            r = rNextRect.Top();
    }
    return r == LONG_MAX ? 0 : r;
}

uno::Reference<text::XTextViewCursor> currentCursor()
{
    SwView* pView = dynamic_cast<SwView*>(SfxViewShell::Current());
    if (!pView)
    {
        emscripten_console_error("missing view!");
        return {};
    }
    uno::Reference<text::XTextViewCursorSupplier> xTextViewCursorSupplier(
        pView->GetCurrentDocument()->getCurrentController(), uno::UNO_QUERY);
    if (!xTextViewCursorSupplier)
    {
        emscripten_console_error("no cursor!");
        return {};
    }
    return xTextViewCursorSupplier->getViewCursor();
}
}

constexpr OUString g_sRecordChanges = u"RecordChanges"_ustr;

val SwXTextDocument::comments()
{
    SolarMutexGuard aGuard;
    val commentsNode = val::array();
    for (auto const& sidebarItem : *m_pDocShell->GetView()->GetPostItMgr())
    {
        sw::annotation::SwAnnotationWin* pWin = sidebarItem->mpPostIt.get();

        if (!pWin)
        {
            continue;
        }

        const SwPostItField* pField = pWin->GetPostItField();
        const SwRect& aRect = pWin->GetAnchorRect();
        tools::Rectangle aSVRect(aRect.Pos().getX(), aRect.Pos().getY(),
                                 aRect.Pos().getX() + aRect.SSize().Width(),
                                 aRect.Pos().getY() + aRect.SSize().Height());

        if (!sidebarItem->maLayoutInfo.mPositionFromCommentAnchor)
        {
            // Comments on frames: anchor position is the corner position, not the whole frame.
            aSVRect.SetSize(Size(0, 0));
        }

        val rects = val::array();
        for (const basegfx::B2DRange& aRange : pWin->GetAnnotationTextRanges())
        {
            const SwRect rect(aRange.getMinX(), aRange.getMinY(), aRange.getWidth(),
                              aRange.getHeight());
            rects.call<void>("push", rectToArray(rect.SVRect()));
        }

        val obj = val::object();
        obj.set("id", val(pField->GetPostItId()));
        obj.set("parentId", val(pField->GetParentPostItId()));
        obj.set("author", val::u16string(pField->GetPar1().getStr()));
        obj.set("text", val::u16string(pField->GetPar2().getStr()));
        obj.set("resolved", val(pField->GetResolved()));
        obj.set("dateTime", val(utl::toISO8601(pField->GetDateTime().GetUNODateTime())));
        obj.set("anchorPos", rectToArray(aSVRect));
        obj.set("textRange", rects);
        obj.set("layoutStatus", val(static_cast<sal_Int16>(pWin->GetLayoutStatus())));

        commentsNode.call<void>("push", obj);
    }

    return commentsNode;
}

void SwXTextDocument::addComment(const std::string& text)
{
    css::uno::Sequence<css::beans::PropertyValue> aPropertyValues(
        comphelper::InitPropertySequence({ { "Text", uno::Any(OUString::fromUtf8(text)) } }));

    SolarMutexGuard aGuard;

    bool recordChanges = getPropertyValue(g_sRecordChanges).get<bool>();
    if (recordChanges)
    {
        setPropertyValue(g_sRecordChanges, uno::Any(false));
    }
    comphelper::dispatchCommand(u".uno:InsertAnnotation"_ustr, aPropertyValues);
    if (recordChanges)
    {
        setPropertyValue(g_sRecordChanges, uno::Any(true));
    }
}

void SwXTextDocument::replyComment(int parentId, const std::string& text)
{
    css::uno::Sequence<css::beans::PropertyValue> aPropertyValues(
        comphelper::InitPropertySequence({ { "Id", uno::Any(static_cast<sal_uInt32>(parentId)) },
                                           { "Text", uno::Any(OUString::fromUtf8(text)) } }));

    SolarMutexGuard aGuard;
    bool recordChanges = getPropertyValue(g_sRecordChanges).get<bool>();
    if (recordChanges)
    {
        setPropertyValue(g_sRecordChanges, uno::Any(false));
    }

    comphelper::dispatchCommand(u".uno:ReplyComment"_ustr, aPropertyValues);

    if (recordChanges)
    {
        setPropertyValue(g_sRecordChanges, uno::Any(true));
    }
}

void SwXTextDocument::deleteCommentThreads(val parentIds)
{
    SwPostItMgr* pMgr = m_pDocShell->GetView()->GetPostItMgr();
    std::vector<sal_uInt32> ids = vecFromJSArray<sal_uInt32>(parentIds);

    SolarMutexGuard aGuard;

    bool recordChanges = getPropertyValue(g_sRecordChanges).get<bool>();
    if (recordChanges)
    {
        setPropertyValue(g_sRecordChanges, uno::Any(false));
    }

    pMgr->DeleteCommentThreads(ids);

    if (recordChanges)
    {
        setPropertyValue(g_sRecordChanges, uno::Any(true));
    }
}

void SwXTextDocument::deleteComment(int commentId)
{
    SwPostItMgr* pMgr = m_pDocShell->GetView()->GetPostItMgr();

    SolarMutexGuard aGuard;

    bool recordChanges = getPropertyValue(g_sRecordChanges).get<bool>();
    if (recordChanges)
    {
        setPropertyValue(g_sRecordChanges, uno::Any(false));
    }

    pMgr->Delete(commentId);

    if (recordChanges)
    {
        setPropertyValue(g_sRecordChanges, uno::Any(true));
    }
}

void SwXTextDocument::resolveCommentThread(int parentId)
{
    css::uno::Sequence<css::beans::PropertyValue> aPropertyValues(comphelper::InitPropertySequence(
        { { "Id", uno::Any(static_cast<sal_uInt32>(parentId)) } }));

    SolarMutexGuard aGuard;

    bool recordChanges = getPropertyValue(g_sRecordChanges).get<bool>();
    if (recordChanges)
    {
        setPropertyValue(g_sRecordChanges, uno::Any(false));
    }

    comphelper::dispatchCommand(u".uno:ResolveCommentThread"_ustr, aPropertyValues);

    if (recordChanges)
    {
        setPropertyValue(g_sRecordChanges, uno::Any(true));
    }
};

void SwXTextDocument::resolveComment(int commentId)
{
    css::uno::Sequence<css::beans::PropertyValue> aPropertyValues(comphelper::InitPropertySequence(
        { { "Id", uno::Any(static_cast<sal_uInt32>(commentId)) } }));

    SolarMutexGuard aGuard;

    bool recordChanges = getPropertyValue(g_sRecordChanges).get<bool>();
    if (recordChanges)
    {
        setPropertyValue(g_sRecordChanges, uno::Any(false));
    }

    comphelper::dispatchCommand(u".uno:ResolveComment"_ustr, aPropertyValues);

    if (recordChanges)
    {
        setPropertyValue(g_sRecordChanges, uno::Any(true));
    }
};

void SwXTextDocument::sanitize(val options)
{
    SolarMutexGuard aGuard;

    if (options["documentMetadata"].isTrue())
    {
        uno::Reference<document::XDocumentPropertiesSupplier> xDPS(m_pDocShell->GetModel(),
                                                                   uno::UNO_QUERY);
        uno::Reference<document::XDocumentProperties> props = xDPS->getDocumentProperties();
        uno::Reference<beans::XPropertyContainer> userDefinedProps
            = props->getUserDefinedProperties();
        uno::Reference<beans::XPropertySet> xPropSet(userDefinedProps, uno::UNO_QUERY);
        if (!xPropSet.is())
            return;
        const uno::Sequence<beans::Property> aProperties
            = xPropSet->getPropertySetInfo()->getProperties();
        for (const beans::Property& rProperty : aProperties)
        {
            userDefinedProps->removeProperty(rProperty.Name);
        }
    }

    if (options["trackChangesAccept"].isTrue())
    {
        comphelper::dispatchCommand(u".uno:AcceptAllTrackedChanges"_ustr, {});
    }
    else if (options["trackChangesReject"].isTrue())
    {
        comphelper::dispatchCommand(u".uno:RejectAllTrackedChanges"_ustr, {});
    }

    if (options["comments"].isTrue())
    {
        bool recordChanges = getPropertyValue(g_sRecordChanges).get<bool>();
        if (recordChanges)
        {
            setPropertyValue(g_sRecordChanges, uno::Any(false));
        }

        comphelper::dispatchCommand(u".uno:DeleteAllNotes"_ustr, {});

        if (recordChanges)
        {
            setPropertyValue(g_sRecordChanges, uno::Any(true));
        }
    }
}

val SwXTextDocument::pageRects()
{
    val result = val::array();

    SolarMutexGuard aGuard;
    SwRootFrame* pLayout = GetDocShell()->GetWrtShell()->GetLayout();
    for (const SwFrame* pFrame = pLayout->GetLower(); pFrame; pFrame = pFrame->GetNext())
    {
        if (pFrame->getFrameArea().Width() > 0 && pFrame->getFrameArea().Height() > 0)
            result.call<void>("push", rectToArray(pFrame->getFrameArea()));
    }

    return result;
}

val SwXTextDocument::headerFooterRect()
{
    SwRect aRect;
    bool bInHeader = true;

    SolarMutexGuard aGuard;
    if (!GetDocShell()->GetWrtShell()->IsInHeaderFooter(&bInHeader, &aRect))
        return val::undefined();
    val result = val::object();
    result.set("type", bInHeader ? val::u8string("header") : val::u8string("footer"));
    result.set("rect", rectToArray(aRect));
    return result;
}

namespace
{
val paragraphStyle(val (*unoAnyToVal)(const css::uno::Any& any), SfxStyleSheetBase* pStyle,
                   const uno::Reference<container::XNameAccess>& xParaStyles, const val& properties,
                   const uno::Sequence<rtl::OUString>& names)
{
    using namespace css::uno;
    Reference<style::XStyle> xStyle(xParaStyles->getByName(pStyle->GetName()), UNO_QUERY);
    Reference<beans::XMultiPropertySet> xStyleProp(xStyle, UNO_QUERY);
    if (!xStyleProp.is())
        return val::undefined();

    val r = val::object();
    r.set("name", val::u16string(pStyle->GetName().getStr()));
    uno::Sequence<uno::Any> values = xStyleProp->getPropertyValues(names);
    Any* valuesArray = values.getArray();
    for (sal_uInt32 i = 0; i < (sal_uInt32)names.getLength(); ++i)
    {
        r.set(properties[i], unoAnyToVal(valuesArray[i]));
    }

    return r;
}
}

val SwXTextDocument::paragraphStyles(val (*unoAnyToVal)(const css::uno::Any& any),
                                     const css::uno::Reference<css::container::XNameAccess> xStyles,
                                     const val& properties,
                                     const css::uno::Sequence<rtl::OUString>& names)
{
    SolarMutexGuard aGuard;

    SfxStyleSheetBasePool* pPool = m_pDocShell->GetStyleSheetPool();
    auto xIter = pPool->CreateIterator(SfxStyleFamily::Para);
    val r = val::object();
    val userDefined = val::array();
    val used = val::array();
    val other = val::array();
    if (xIter->Count() <= 1)
    {
        SAL_WARN("wasm", "could not retrieve style pool");
        return val::undefined();
    }

    SfxStyleSheetBase* pStyle = xIter->First();
    while (pStyle)
    {
        if (pStyle->IsHidden())
        {
            pStyle = xIter->Next();
            continue;
        }
        val style = paragraphStyle(unoAnyToVal, pStyle, xStyles, properties, names);
        if (pStyle->IsUserDefined())
        {
            userDefined.call<void>("push", style);
        }
        else if (pStyle->IsUsed())
        {
            used.call<void>("push", style);
        }
        else
        {
            other.call<void>("push", style);
        }

        pStyle = xIter->Next();
    }
    r.set("userDefined", userDefined);
    r.set("used", used);
    r.set("other", other);

    return r;
}

class TextRanges_Impl final : public wasm::ITextRanges
{
private:
    std::vector<rtl::Reference<SwXTextRange>> ranges_;
    std::vector<tools::Long> cachedBottomTwips_;
    sw::UnoCursorPointer unoCursorPtr_;
    const OUString searchString_;

    SwRects* rangeRects(SwWrtShell* const pWrtShell, int index)
    {
        val rArray = val::array();
        rtl::Reference<SwXTextRange> range = ranges_.at(index);
        SwUnoInternalPaM aPam(range->GetDoc());
        if (!range->GetPositions(aPam))
        {
            emscripten_console_error("missing PaM");
            return nullptr;
        }

        SwPosition* startPos = aPam.Start();
        SwPosition* endPos = aPam.End();
        SwContentNode* node = aPam.GetPointContentNode();
        if (!startPos || !endPos || !node)
        {
            emscripten_console_error("missing node");
            return nullptr;
        }

        SwShellCursor aCursor(*pWrtShell, *startPos);
        aCursor.SetMark();
        aCursor.GetMark()->Assign(*node, endPos->GetContentIndex());

        aCursor.FillRects();

        SwRects* pRects(&aCursor);
        cachedBottomTwips_[index] = bottomTwips(pRects);
        return pRects;
    }

    bool isOutOfRange(int index) { return index < 0 || index >= (int)ranges_.size(); }

    void replace(const rtl::Reference<SwXTextRange>& range, const OUString& replaceString)
    {
        SwDoc& rDoc = range->GetDoc();
        SwUnoInternalPaM aPaM(rDoc);
        if (!range->GetPositions(aPaM))
        {
            emscripten_console_error("missing PaM");
            return;
        }
        if (aPaM.HasMark() && *aPaM.GetMark() != *aPaM.GetPoint())
        {
            // TODO: support regexp?
            sw::ReplaceImpl(aPaM, replaceString, /* regex */ false, rDoc,
                            rDoc.getIDocumentLayoutAccess().GetCurrentLayout());
        }
    }

    val description(const rtl::Reference<SwXTextRange>& range)
    {
        static constexpr int WORDS_BEFORE = 2;
        static constexpr int WORDS_AFTER = 4;

        val r = val::array();
        uno::Reference<text::XWordCursor> startCursor(
            range->getText()->createTextCursorByRange(range), uno::UNO_QUERY_THROW);
        uno::Reference<text::XWordCursor> endCursor(
            range->getText()->createTextCursorByRange(range), uno::UNO_QUERY_THROW);

        // beginning of the word, then select two words before
        startCursor->goLeft(0, false);
        for (int i = 0; i < WORDS_BEFORE; ++i)
            startCursor->gotoPreviousWord(true);
        for (int i = 0; i < WORDS_AFTER; ++i)
            endCursor->gotoNextWord(true);

        r.call<void>("push", val::u16string(startCursor->getString().getStr()),
                     val::u16string(range->getString().getStr()),
                     val::u16string(endCursor->getString().getStr()));

        return r;
    }

    bool rangeContainsCursor(int index, uno::Reference<text::XTextRange> cursor)
    {
        uno::Reference<text::XTextRange> range = ranges_.at(index);
        uno::Reference<text::XTextRangeCompare> compare(range->getText(), uno::UNO_QUERY_THROW);
        return compare->compareRegionStarts(cursor, range) >= 0
               || compare->compareRegionEnds(cursor, range) <= 0;
    }

public:
    TextRanges_Impl(SwPaM* const pPaM, OUString searchString)
        : searchString_(searchString)
    {
        if (pPaM)
        {
            unoCursorPtr_.reset(pPaM->GetDoc().CreateUnoCursor(*pPaM->GetPoint()));
            ::sw::DeepCopyPaM(*pPaM, *unoCursorPtr_);
        }
        if (!unoCursorPtr_)
            return;

        for (SwPaM& rTmpCursor : unoCursorPtr_->GetRingContainer())
        {
            const rtl::Reference<SwXTextRange> xRange(SwXTextRange::CreateXTextRange(
                rTmpCursor.GetDoc(), *rTmpCursor.GetPoint(), rTmpCursor.GetMark()));
            if (xRange.is())
            {
                ranges_.push_back(xRange);
            }
        }
        cachedBottomTwips_.resize(ranges_.size());
    }

    int length() override { return ranges_.size(); };

    val rect(int index) override
    {
        if (isOutOfRange(index))
            return val::undefined();

        SolarMutexGuard aGuard;
        SwView* pView = dynamic_cast<SwView*>(SfxViewShell::Current());
        if (!pView)
        {
            emscripten_console_error("missing view!");
            return val::undefined();
        }
        SwRects* rects = rangeRects(pView->GetWrtShellPtr(), index);
        if (!rects)
            return val::undefined();
        return swRectsToArray(rects);
    };

    val rects(int startYPosTwips, int endYPosTwips) override
    {
        SolarMutexGuard aGuard;
        SwView* pView = dynamic_cast<SwView*>(SfxViewShell::Current());
        if (!pView)
        {
            emscripten_console_error("missing view!");
            return val::undefined();
        }
        SwWrtShell* pWrtShell = pView->GetWrtShellPtr();
        if (!pWrtShell)
        {
            emscripten_console_error("missing shell!");
            return val::undefined();
        }

        // predicting based on the cached bottom-most coordinate prevents running layout, which is expensive
        int predictedStartIndex = -1;
        int predictedEndIndex = -1;
        for (int i = 0; i < (int)cachedBottomTwips_.size(); ++i)
        {
            tools::Long cachedBottomTwips = cachedBottomTwips_.at(i);
            if (cachedBottomTwips == 0)
                continue;

            SwRects* pRects = rangeRects(pView->GetWrtShellPtr(), i);
            if (!pRects)
                return val::undefined();

            if (startYPosTwips <= cachedBottomTwips)
            {
                // if the cached bottom doesn't match the current bottom, a change has occurred and the values aren't valid
                if (cachedBottomTwips != bottomTwips(pRects))
                    break;

                predictedStartIndex = i;
            }

            if (endYPosTwips >= cachedBottomTwips)
            {
                if (cachedBottomTwips != bottomTwips(pRects))
                {
                    predictedStartIndex = -1;
                    break;
                }

                predictedEndIndex = i;
            }
        }

        val r = val::array();
        const int startIndex = std::clamp(predictedStartIndex, 0, (int)ranges_.size() - 1);
        const int endIndex = std::clamp(predictedEndIndex, startIndex, (int)ranges_.size() - 1);
        for (int i = startIndex; i <= endIndex; ++i)
        {
            SwRects* pRects = rangeRects(pView->GetWrtShellPtr(), i);
            if (!pRects)
                return val::undefined();

            if (bottomTwips(pRects) < startYPosTwips)
                continue;
            if (topTwips(pRects) > endYPosTwips)
                continue;
            val o = val::object();
            o.set("i", i);
            o.set("rect", swRectsToArray(pRects));
            r.call<void>("push", o);
        }

        return r;
    };

    bool isCursorAt(int index) override
    {
        if (isOutOfRange(index))
            return false;
        SolarMutexGuard aGuard;
        uno::Reference<text::XTextViewCursor> xCursor = currentCursor();
        if (!xCursor.is())
        {
            emscripten_console_error("no cursor!");
            return false;
        }
        return rangeContainsCursor(index, xCursor);
    };

    int indexAtCursor() override
    {
        SolarMutexGuard aGuard;
        uno::Reference<text::XTextViewCursor> xCursor = currentCursor();

        for (int i = 0; i < (int)ranges_.size(); ++i)
        {
            if (rangeContainsCursor(i, xCursor))
                return i;
        }
        return -1;
    };

    void moveCursorTo(int index, bool end, bool select) override
    {
        SolarMutexGuard aGuard;
        if (isOutOfRange(index))
            return;

        uno::Reference<text::XTextViewCursor> xCursor = currentCursor();
        if (!xCursor.is())
        {
            emscripten_console_error("no cursor!");
            return;
        }

        xCursor->gotoRange(ranges_.at(index), select);
        if (end)
            xCursor->gotoRange(ranges_.at(0)->getEnd(), select);
    }

    val description(int index) override
    {
        if (isOutOfRange(index))
            return val::undefined();
        SolarMutexGuard aGuard;

        return description(ranges_.at(index));
    };

    val descriptions(int startIndex, int endIndex) override
    {
        startIndex = std::clamp(startIndex, 0, (int)ranges_.size() - 1);
        endIndex = std::clamp(endIndex, 0, (int)ranges_.size() - 1);

        SolarMutexGuard aGuard;

        val r = val::array();
        for (int i = startIndex; i <= endIndex; ++i)
        {
            val o = val::object();
            o.set("i", i);
            o.set("desc", description(ranges_.at(i)));

            r.call<void>("push", o);
        }

        return r;
    };

    void replace(int index, const std::string& text) override
    {
        if (isOutOfRange(index))
            return;

        SolarMutexGuard aGuard;

        OUString replaceString = OUString::fromUtf8(text);
        auto range = ranges_.at(0);
        SwDoc& rDoc = range->GetDoc();
        UnoActionContext aAction(&rDoc);
        rDoc.GetIDocumentUndoRedo().StartUndo(SwUndoId::REPLACE, nullptr);

        replace(range, replaceString);

        SwRewriter rewriter(MakeUndoReplaceRewriter(ranges_.size(), searchString_, replaceString));
        rDoc.GetIDocumentUndoRedo().EndUndo(SwUndoId::REPLACE, &rewriter);
    };

    void replaceAll(const std::string& text) override
    {
        if (ranges_.empty())
            return;

        SolarMutexGuard aGuard;

        OUString replaceString = OUString::fromUtf8(text);

        SwDoc& rDoc = ranges_.at(0)->GetDoc();
        UnoActionContext aAction(&rDoc);
        rDoc.GetIDocumentUndoRedo().StartUndo(SwUndoId::REPLACE, nullptr);

        for (const auto& range : ranges_)
        {
            replace(range, replaceString);
        }

        SwRewriter rewriter(MakeUndoReplaceRewriter(ranges_.size(), searchString_, replaceString));
        rDoc.GetIDocumentUndoRedo().EndUndo(SwUndoId::REPLACE, &rewriter);
    };
};

std::shared_ptr<wasm::ITextRanges> SwXTextDocument::findAllTextRanges(const std::string& text,
                                                                      val flags)
{
    SolarMutexGuard aGuard;

    uno::Reference<util::XSearchDescriptor> xSearch = createSearchDescriptor();
    OUString searchString = OUString::fromUtf8(text);
    xSearch->setSearchString(searchString);
    if (!flags.isUndefined() && !flags.isNull())
    {
        uno::Reference<beans::XPropertySet> xFlags(xSearch, uno::UNO_QUERY_THROW);
        if (flags["caseSensitive"].isTrue())
        {
            xFlags->setPropertyValue("SearchCaseSensitive", uno::Any(true));
        }
        if (flags["wholeWords"].isTrue())
        {
            xFlags->setPropertyValue("SearchWords", uno::Any(true));
        }
        if (flags["mode"].isString())
        {
            auto mode = flags["mode"].as<std::string>();
            if (mode == "wildcard")
            {
                xFlags->setPropertyValue("SearchWildcard", uno::Any(true));
            }
            else if (mode == "regex")
            {
                xFlags->setPropertyValue("SearchRegularExpression", uno::Any(true));
            }
            else if (mode == "similar")
            {
                xFlags->setPropertyValue("SearchSimilarity", uno::Any(true));
            }
        }
    }
    uno::Reference<uno::XInterface> xTmp;
    sal_Int32 nResult = 0;
    uno::Reference<text::XTextCursor> xCursor;
    SwUnoCursor* pResultCursor(FindAny(xSearch, xCursor, true, nResult, xTmp));
    if (!pResultCursor)
    {
        emscripten_console_error("no result cursor");
        return {};
    }

    return std::make_shared<TextRanges_Impl>(pResultCursor, searchString);
}

void SwXTextDocument::cancelFindOrReplace()
{
    sw::BumpSearchGeneration();
    SwUndoId pId;

    SolarMutexGuard aGuard;
    IDocumentUndoRedo& rUndoRedo = GetDocOrThrow().GetIDocumentUndoRedo();
    rUndoRedo.GetLastUndoInfo(nullptr, &pId);
    // canceling replace part way is not a desirable state and so any changes should be undone
    if (pId == SwUndoId::REPLACE)
    {
        rUndoRedo.Undo();
    }
}

val SwXTextDocument::getOutline()
{
    SolarMutexGuard aGuard;

    SwWrtShell* mrSh = m_pDocShell->GetWrtShell();
    if (!mrSh)
    {
        emscripten_console_error("no shell");
        return {};
    }

    const SwOutlineNodes::size_type nOutlineCount
        = mrSh->getIDocumentOutlineNodesAccess()->getOutlineNodesCount();

    typedef std::pair<sal_Int8, sal_Int32> StackEntry;
    std::stack<StackEntry> aOutlineStack;
    aOutlineStack.push(StackEntry(-1, -1)); // push default value

    int nOutlineId = 0;

    val r = val::array();
    const SwOutlineNodes& rNodes = mrSh->GetNodes().GetOutLineNds();
    for (SwOutlineNodes::size_type i = 0; i < nOutlineCount; ++i)
    {
        // Check if outline is hidden
        const SwTextNode* textNode = rNodes[i]->GetTextNode();

        if (textNode->IsHidden() || !sw::IsParaPropsNode(*mrSh->GetLayout(), *textNode) ||
            // Skip empty outlines:
            textNode->GetText().isEmpty())
        {
            continue;
        }

        // Get parent id from stack:
        const sal_Int8 nLevel
            = static_cast<sal_Int8>(mrSh->getIDocumentOutlineNodesAccess()->getOutlineLevel(i));

        sal_Int8 nLevelOnTopOfStack = aOutlineStack.top().first;
        while (nLevelOnTopOfStack >= nLevel && nLevelOnTopOfStack != -1)
        {
            aOutlineStack.pop();
            nLevelOnTopOfStack = aOutlineStack.top().first;
        }

        const sal_Int32 nParent = aOutlineStack.top().second;

        val o = val::object();
        o.set("id", nOutlineId);
        o.set("parent", nParent);
        o.set("text", textNode->GetText());
        r.call<void>("push", o);

        aOutlineStack.push(StackEntry(nLevel, nOutlineId));

        nOutlineId++;
    }

    return r;
}

val SwXTextDocument::gotoOutline(int outlineIndex)
{
    SolarMutexGuard aGuard;

    SwWrtShell* mrSh = m_pDocShell->GetWrtShell();
    if (!mrSh)
    {
        emscripten_console_error("no shell");
        return {};
    }

    mrSh->GotoOutline(outlineIndex);

    return rectToArray(mrSh->GetCharRect());
}

namespace sw
{
// search generations are used instead of a boolean because a simple boolean cannot gaurantee ordering
_Atomic int g_searchGeneration = 0;

void BumpSearchGeneration() { __c11_atomic_fetch_add(&g_searchGeneration, 1, __ATOMIC_RELAXED); }

int GetSearchGeneration() { return __c11_atomic_load(&g_searchGeneration, __ATOMIC_RELAXED); }

}
