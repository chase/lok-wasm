/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <libxml/xmlwriter.h>

#include <IShellCursorSupplier.hxx>
#include <txtftn.hxx>
#include <fmtanchr.hxx>
#include <ftnidx.hxx>
#include <frmfmt.hxx>
#include <doc.hxx>
#include <UndoManager.hxx>
#include <IDocumentRedlineAccess.hxx>
#include <docary.hxx>
#include <swcrsr.hxx>
#include <swundo.hxx>
#include <pam.hxx>
#include <ndtxt.hxx>
#include <UndoCore.hxx>
#include <rolbck.hxx>
#include <ndnotxt.hxx>
#include <IMark.hxx>
#include <mvsave.hxx>
#include <redline.hxx>
#include <crossrefbookmark.hxx>
#include <strings.hrc>
#include <docsh.hxx>
#include <view.hxx>
#include <frameformats.hxx>
#include <o3tl/deleter.hxx>
#include <sal/log.hxx>

// This class saves the Pam as integers and can recompose those into a PaM
SwUndRng::SwUndRng()
    : m_nSttNode( 0 ), m_nEndNode( 0 ), m_nSttContent( 0 ), m_nEndContent( 0 )
{
}

SwUndRng::SwUndRng( const SwPaM& rPam )
{
    SetValues( rPam );
}

void SwUndRng::SetValues( const SwPaM& rPam )
{
    const SwPosition *pStt = rPam.Start();
    if( rPam.HasMark() )
    {
        const SwPosition *pEnd = rPam.End();
        m_nEndNode = pEnd->GetNodeIndex();
        m_nEndContent = pEnd->GetContentIndex();
    }
    else
    {
        // no selection !!
        m_nEndNode = SwNodeOffset(0);
        m_nEndContent = COMPLETE_STRING;
    }

    m_nSttNode = pStt->GetNodeIndex();
    m_nSttContent = pStt->GetContentIndex();
}

void SwUndRng::SetPaM( SwPaM & rPam, bool bCorrToContent ) const
{
    rPam.DeleteMark();
    rPam.GetPoint()->Assign( m_nSttNode, m_nSttContent );
    SwNode& rNd = rPam.GetPointNode();
    if( !rNd.IsContentNode() && bCorrToContent )
        rPam.Move( fnMoveForward, GoInContent );

    if( !m_nEndNode && COMPLETE_STRING == m_nEndContent )       // no selection
        return ;

    rPam.SetMark();
    if( m_nSttNode == m_nEndNode && m_nSttContent == m_nEndContent )
        return;                             // nothing left to do

    rPam.GetPoint()->Assign( m_nEndNode, m_nEndContent );
    if( !rPam.GetPointNode().IsContentNode() && bCorrToContent )
        rPam.Move( fnMoveBackward, GoInContent );
}

SwPaM & SwUndRng::AddUndoRedoPaM(
        ::sw::UndoRedoContext & rContext, bool const bCorrToContent) const
{
    SwCursor & rPaM( rContext.GetCursorSupplier().CreateNewShellCursor() );
    SetPaM( rPaM, bCorrToContent );
    return rPaM;
}

void SwUndo::RemoveIdxFromSection( SwDoc& rDoc, SwNodeOffset nSttIdx,
                                    const SwNodeOffset* pEndIdx )
{
    SwNodeIndex aIdx( rDoc.GetNodes(), nSttIdx );
    SwNodeIndex aEndIdx( rDoc.GetNodes(), pEndIdx ? *pEndIdx
                                    : aIdx.GetNode().EndOfSectionIndex() );
    SwPosition aPos( rDoc.GetNodes().GetEndOfPostIts() );
    SwDoc::CorrAbs( aIdx, aEndIdx, aPos, true );
}

void SwUndo::RemoveIdxFromRange( SwPaM& rPam, bool bMoveNext )
{
    const SwPosition* pEnd = rPam.End();
    if( bMoveNext )
    {
        if( pEnd != rPam.GetPoint() )
            rPam.Exchange();

        SwNodeIndex aStt( rPam.GetMark()->GetNode() );
        SwNodeIndex aEnd( rPam.GetPoint()->GetNode() );

        if( !rPam.Move( fnMoveForward ) )
        {
            rPam.Exchange();
            if( !rPam.Move( fnMoveBackward ) )
            {
                rPam.GetPoint()->Assign( rPam.GetDoc().GetNodes().GetEndOfPostIts() );
            }
        }

        SwDoc::CorrAbs( aStt, aEnd, *rPam.GetPoint(), true );
    }
    else
        SwDoc::CorrAbs( rPam, *pEnd, true );
}

void SwUndo::RemoveIdxRel( SwNodeOffset nIdx, const SwPosition& rPos )
{
    // Move only the Cursor. Bookmarks/TOXMarks/etc. are done by the corresponding
    // JoinNext/JoinPrev
    ::PaMCorrRel( *rPos.GetNode().GetNodes()[nIdx], rPos );
}

SwUndo::SwUndo(SwUndoId const nId, const SwDoc* pDoc)
    : m_nId(nId), m_nOrigRedlineFlags(RedlineFlags::NONE)
    , m_nViewShellId(CreateViewShellId(pDoc))
    , m_isRepeatIgnored(false)
    , m_bCacheComment(true)
{
}

ViewShellId SwUndo::CreateViewShellId(const SwDoc* pDoc)
{
    ViewShellId nRet(-1);

    if (const SwDocShell* pDocShell = pDoc->GetDocShell())
    {
        if (const SwView* pView = pDocShell->GetView())
            nRet = pView->GetViewShellId();
    }

    return nRet;
}

bool SwUndo::IsDelBox() const
{
    return GetId() == SwUndoId::COL_DELETE || GetId() == SwUndoId::ROW_DELETE ||
        GetId() == SwUndoId::TABLE_DELBOX;
}

SwUndo::~SwUndo()
{
}

namespace {

class UndoRedoRedlineGuard
{
public:
    UndoRedoRedlineGuard(::sw::UndoRedoContext const & rContext, SwUndo const & rUndo)
        : m_rRedlineAccess(rContext.GetDoc().getIDocumentRedlineAccess())
        , m_eMode(m_rRedlineAccess.GetRedlineFlags())
    {
        RedlineFlags const eTmpMode = rUndo.GetRedlineFlags();
        if ((RedlineFlags::ShowMask & eTmpMode) != (RedlineFlags::ShowMask & m_eMode))
        {
            m_rRedlineAccess.SetRedlineFlags( eTmpMode );
        }
        m_rRedlineAccess.SetRedlineFlags_intern( eTmpMode | RedlineFlags::Ignore );
    }
    ~UndoRedoRedlineGuard()
    {
        m_rRedlineAccess.SetRedlineFlags(m_eMode);
    }
private:
    IDocumentRedlineAccess & m_rRedlineAccess;
    RedlineFlags const m_eMode;
};

}

void SwUndo::Undo()
{
    assert(false); // SwUndo::Undo(): ERROR: must call UndoWithContext instead
}

void SwUndo::Redo()
{
    assert(false); // SwUndo::Redo(): ERROR: must call RedoWithContext instead
}

void SwUndo::UndoWithContext(SfxUndoContext & rContext)
{
    ::sw::UndoRedoContext *const pContext(
            dynamic_cast< ::sw::UndoRedoContext * >(& rContext));
    assert(pContext);
    const UndoRedoRedlineGuard aUndoRedoRedlineGuard(*pContext, *this);
    UndoImpl(*pContext);
}

void SwUndo::RedoWithContext(SfxUndoContext & rContext)
{
    ::sw::UndoRedoContext *const pContext(
            dynamic_cast< ::sw::UndoRedoContext * >(& rContext));
    assert(pContext);
    const UndoRedoRedlineGuard aUndoRedoRedlineGuard(*pContext, *this);
    RedoImpl(*pContext);
}

void SwUndo::Repeat(SfxRepeatTarget & rContext)
{
    if (m_isRepeatIgnored)
    {
        return; // ignore Repeat for multi-selections
    }
    ::sw::RepeatContext *const pRepeatContext(
            dynamic_cast< ::sw::RepeatContext * >(& rContext));
    assert(pRepeatContext);
    RepeatImpl(*pRepeatContext);
}

bool SwUndo::CanRepeat(SfxRepeatTarget & rContext) const
{
    assert(dynamic_cast< ::sw::RepeatContext * >(& rContext));
    (void)rContext;
    // a MultiSelection action that doesn't do anything must still return true
    return (SwUndoId::REPEAT_START <= GetId()) && (GetId() < SwUndoId::REPEAT_END);
}

void SwUndo::RepeatImpl( ::sw::RepeatContext & )
{
}

// MACRO-2601: Make it easier to know which action {
OUString GetUndoComment(SwUndoId eId)
{
    return OUString::number(static_cast<sal_uInt32>(eId));
}
// MACRO-2601: }

OUString SwUndo::GetComment() const
{
    // MACRO-2601: Make it easier to know which action {
    if (! maComment)
    {
        maComment = OUString::number(static_cast<sal_uInt32>(GetId()));
    }
    return *maComment;
    // MACRO-2601: }
}

ViewShellId SwUndo::GetViewShellId() const
{
    return m_nViewShellId;
}

SwRewriter SwUndo::GetRewriter() const
{
    SwRewriter aResult;

    return aResult;
}

SwUndoSaveContent::SwUndoSaveContent()
{}

SwUndoSaveContent::~SwUndoSaveContent() COVERITY_NOEXCEPT_FALSE
{
}

void SwUndoSaveContent::dumpAsXml(xmlTextWriterPtr pWriter) const
{
    (void)xmlTextWriterStartElement(pWriter, BAD_CAST("SwUndoSaveContent"));
    (void)xmlTextWriterWriteFormatAttribute(pWriter, BAD_CAST("ptr"), "%p", this);

    if (m_pHistory)
    {
        m_pHistory->dumpAsXml(pWriter);
    }

    (void)xmlTextWriterEndElement(pWriter);
}

// This is needed when deleting content. For REDO all contents will be moved
// into the UndoNodesArray. These methods always create a new node to insert
// content. As a result, the attributes will not be expanded.
// - MoveTo   moves from NodesArray into UndoNodesArray
// - MoveFrom moves from UndoNodesArray into NodesArray

// If pEndNdIdx is given, Undo/Redo calls -Ins/DelFly. In that case the whole
// section should be moved.
void SwUndoSaveContent::MoveToUndoNds( SwPaM& rPaM, SwNodeIndex* pNodeIdx,
                    SwNodeOffset* pEndNdIdx )
{
    SwDoc& rDoc = rPaM.GetDoc();
    ::sw::UndoGuard const undoGuard(rDoc.GetIDocumentUndoRedo());

    SwNoTextNode* pCpyNd = rPaM.GetPointNode().GetNoTextNode();

    // here comes the actual delete (move)
    SwNodes & rNds = rDoc.GetUndoManager().GetUndoNodes();
    SwPosition aPos( pEndNdIdx ? rNds.GetEndOfPostIts()
                               : rNds.GetEndOfExtras() );

    const SwPosition* pStt = rPaM.Start(), *pEnd = rPaM.End();

    SwNodeOffset nTmpMvNode = aPos.GetNodeIndex();

    if( pCpyNd || pEndNdIdx )
    {
        SwNodeRange aRg( pStt->GetNode(), SwNodeOffset(0), pEnd->GetNode(), SwNodeOffset(1) );
        rDoc.GetNodes().MoveNodes( aRg, rNds, aPos.GetNode(), true );
        aPos.Adjust(SwNodeOffset(-1));
    }
    else
    {
        rDoc.GetNodes().MoveRange( rPaM, aPos, rNds );
    }
    if( pEndNdIdx )
        *pEndNdIdx = aPos.GetNodeIndex();

    // old position
    aPos.Assign(nTmpMvNode);
    if( pNodeIdx )
        *pNodeIdx = aPos.GetNode();
}

void SwUndoSaveContent::MoveFromUndoNds( SwDoc& rDoc, SwNodeOffset nNodeIdx,
                            SwPosition& rInsPos,
            const SwNodeOffset* pEndNdIdx, bool const bForceCreateFrames)
{
    // here comes the recovery
    SwNodes & rNds = rDoc.GetUndoManager().GetUndoNodes();
    if( nNodeIdx == rNds.GetEndOfPostIts().GetIndex() )
        return;     // nothing saved

    ::sw::UndoGuard const undoGuard(rDoc.GetIDocumentUndoRedo());

    SwPaM aPaM( rInsPos );
    if( pEndNdIdx )         // than get the section from it
        aPaM.GetPoint()->Assign( *rNds[SwNodeOffset(0)], *pEndNdIdx );
    else
    {
        aPaM.GetPoint()->Assign( rNds.GetEndOfExtras() );
        GoInContent( aPaM, fnMoveBackward );
    }

    SwTextNode* pTextNd = aPaM.GetPointNode().GetTextNode();
    if (!pEndNdIdx && pTextNd)
    {
        aPaM.SetMark();
        aPaM.GetPoint()->Assign(nNodeIdx, 0);

        SaveRedlEndPosForRestore aRedlRest( rInsPos.GetNode(), rInsPos.GetContentIndex() );

        rNds.MoveRange( aPaM, rInsPos, rDoc.GetNodes() );

        // delete the last Node as well
        bool bDeleteLastNode = false;
        if( !aPaM.GetPoint()->GetContentIndex() )
            bDeleteLastNode = true;
        else
        {
            // still empty Nodes at the end?
            aPaM.GetPoint()->Adjust(SwNodeOffset(1));
            if ( &rNds.GetEndOfExtras() != &aPaM.GetPoint()->GetNode() )
                bDeleteLastNode = true;
        }
        if( bDeleteLastNode )
        {
            SwNode& rDelNode = aPaM.GetPoint()->GetNode();
            SwNodeOffset nDelOffset = rNds.GetEndOfExtras().GetIndex() -
                        aPaM.GetPoint()->GetNodeIndex();
            //move it so we don't have SwContentIndex pointing at a node when it is deleted.
            aPaM.GetPoint()->Adjust(SwNodeOffset(-1));
            aPaM.SetMark();
            rNds.Delete( rDelNode, nDelOffset );
        }

        aRedlRest.Restore();
    }
    else
    {
        SwNodeRange aRg( rNds, nNodeIdx, (pEndNdIdx
                        ? ((*pEndNdIdx) + 1)
                        : rNds.GetEndOfExtras().GetIndex() ) );
        rNds.MoveNodes(aRg, rDoc.GetNodes(), rInsPos.GetNode(), nullptr == pEndNdIdx || bForceCreateFrames);

    }
}

// These two methods save and restore the Point of PaM.
// If the point cannot be moved, a "backup" is created on the previous node.
// Either way, returned, inserting at its original position will not move it.
::std::optional<SwNodeIndex> SwUndoSaveContent::MovePtBackward(SwPaM & rPam)
{
    rPam.SetMark();
    if( rPam.Move( fnMoveBackward ))
        return {};

    return { SwNodeIndex(rPam.GetPoint()->GetNode(), -1) };
}

void SwUndoSaveContent::MovePtForward(SwPaM& rPam, ::std::optional<SwNodeIndex> && oMvBkwrd)
{
    // Was there content before this position?
    if (!oMvBkwrd)
        rPam.Move( fnMoveForward );
    else
    {
        *rPam.GetPoint() = SwPosition(*oMvBkwrd);
        rPam.GetPoint()->Adjust(SwNodeOffset(1));
        SwContentNode* pCNd = rPam.GetPointContentNode();
        if( !pCNd )
            rPam.Move( fnMoveForward );
    }
}

// Delete all objects that have ContentIndices to the given area.
// Currently (1994) these exist:
//                  - Footnotes
//                  - Flys
//                  - Bookmarks

// #i81002# - extending method
// delete certain (not all) cross-reference bookmarks at text node of <rMark>
// and at text node of <rPoint>, if these text nodes aren't the same.
void SwUndoSaveContent::DelContentIndex( const SwPosition& rMark,
                                     const SwPosition& rPoint,
                                     DelContentType nDelContentType )
{
    const SwPosition *pStt = rMark < rPoint ? &rMark : &rPoint,
                    *pEnd = &rMark == pStt ? &rPoint : &rMark;

    SwDoc& rDoc = rMark.GetNode().GetDoc();

    // if it's not in the doc array, probably missing some invalidation somewhere
    assert(&rPoint.GetNodes() == &rDoc.GetNodes());
    assert(&rMark.GetNodes() == &rDoc.GetNodes());

    ::sw::UndoGuard const undoGuard(rDoc.GetIDocumentUndoRedo());

    // 1. Footnotes
    if( DelContentType::Ftn & nDelContentType )
    {
        SwFootnoteIdxs& rFootnoteArr = rDoc.GetFootnoteIdxs();
        if( !rFootnoteArr.empty() )
        {
            const SwNode* pFootnoteNd;
            size_t nPos = 0;
            rFootnoteArr.SeekEntry( pStt->GetNode(), &nPos );
            SwTextFootnote* pSrch;

            // for now delete all that come afterwards
            while( nPos < rFootnoteArr.size() && ( pFootnoteNd =
                &( pSrch = rFootnoteArr[ nPos ] )->GetTextNode())->GetIndex()
                        <= pEnd->GetNodeIndex() )
            {
                const sal_Int32 nFootnoteSttIdx = pSrch->GetStart();
                if( (DelContentType::CheckNoCntnt & nDelContentType )
                    ? (&pEnd->GetNode() == pFootnoteNd )
                    : (( &pStt->GetNode() == pFootnoteNd &&
                    pStt->GetContentIndex() > nFootnoteSttIdx) ||
                    ( &pEnd->GetNode() == pFootnoteNd &&
                    nFootnoteSttIdx >= pEnd->GetContentIndex() )) )
                {
                    ++nPos;     // continue searching
                    continue;
                }

// FIXME: duplicated code here and below -> refactor?
                // Unfortunately an index needs to be created. Otherwise there
                // will be problems with TextNode because the index will be
                // deleted in the DTOR of SwFootnote!
                SwTextNode* pTextNd = const_cast<SwTextNode*>(static_cast<const SwTextNode*>(pFootnoteNd));
                if( !m_pHistory )
                    m_pHistory.reset( new SwHistory );
                SwTextAttr* const pFootnoteHint =
                    pTextNd->GetTextAttrForCharAt( nFootnoteSttIdx );
                assert(pFootnoteHint);
                SwContentIndex aIdx( pTextNd, nFootnoteSttIdx );
                m_pHistory->AddTextAttr(pFootnoteHint, pTextNd->GetIndex(), false);
                pTextNd->EraseText( aIdx, 1 );
            }

            while( nPos-- && ( pFootnoteNd = &( pSrch = rFootnoteArr[ nPos ] )->
                    GetTextNode())->GetIndex() >= pStt->GetNodeIndex() )
            {
                const sal_Int32 nFootnoteSttIdx = pSrch->GetStart();
                if( !(DelContentType::CheckNoCntnt & nDelContentType) && (
                    ( &pStt->GetNode() == pFootnoteNd &&
                    pStt->GetContentIndex() > nFootnoteSttIdx ) ||
                    ( &pEnd->GetNode() == pFootnoteNd &&
                    nFootnoteSttIdx >= pEnd->GetContentIndex() )))
                    continue;               // continue searching

                // Unfortunately an index needs to be created. Otherwise there
                // will be problems with TextNode because the index will be
                // deleted in the DTOR of SwFootnote!
                SwTextNode* pTextNd = const_cast<SwTextNode*>(static_cast<const SwTextNode*>(pFootnoteNd));
                if( !m_pHistory )
                    m_pHistory.reset( new SwHistory );
                SwTextAttr* const pFootnoteHint =
                    pTextNd->GetTextAttrForCharAt( nFootnoteSttIdx );
                assert(pFootnoteHint);
                SwContentIndex aIdx( pTextNd, nFootnoteSttIdx );
                m_pHistory->AddTextAttr(pFootnoteHint, pTextNd->GetIndex(), false);
                pTextNd->EraseText( aIdx, 1 );
            }
        }
    }

    // 2. Flys
    if( DelContentType::Fly & nDelContentType )
    {
        sal_uInt16 nChainInsPos = m_pHistory ? m_pHistory->Count() : 0;
        const sw::SpzFrameFormats& rSpzArr = *rDoc.GetSpzFrameFormats();
        if( !rSpzArr.empty() )
        {
            sw::SpzFrameFormat* pFormat;
            const SwFormatAnchor* pAnchor;
            size_t n = rSpzArr.size();
            const SwPosition* pAPos;

            while( n && !rSpzArr.empty() )
            {
                pFormat = rSpzArr[--n];
                pAnchor = &pFormat->GetAnchor();
                switch( pAnchor->GetAnchorId() )
                {
                case RndStdIds::FLY_AS_CHAR:
                    if( nullptr != (pAPos = pAnchor->GetContentAnchor() ) &&
                        (( DelContentType::CheckNoCntnt & nDelContentType )
                        ? ( pStt->GetNode() <= pAPos->GetNode() &&
                            pAPos->GetNode() < pEnd->GetNode() )
                        : ( *pStt <= *pAPos && *pAPos < *pEnd )) )
                    {
                        if( !m_pHistory )
                            m_pHistory.reset( new SwHistory );
                        SwTextNode *const pTextNd =
                            pAPos->GetNode().GetTextNode();
                        SwTextAttr* const pFlyHint = pTextNd->GetTextAttrForCharAt(
                            pAPos->GetContentIndex());
                        assert(pFlyHint);
                        m_pHistory->AddTextAttr(pFlyHint, SwNodeOffset(0), false);
                        // reset n so that no Format is skipped
                        n = n >= rSpzArr.size() ? rSpzArr.size() : n+1;
                    }
                    break;
                case RndStdIds::FLY_AT_PARA:
                    {
                        pAPos =  pAnchor->GetContentAnchor();
                        if (pAPos &&
                            pStt->GetNode() <= pAPos->GetNode() && pAPos->GetNode() <= pEnd->GetNode())
                        {
                            if (!m_pHistory)
                                m_pHistory.reset( new SwHistory );

                            if (!(DelContentType::Replace & nDelContentType)
                                && IsSelectFrameAnchoredAtPara(*pAPos, *pStt, *pEnd, nDelContentType))
                            {
                                m_pHistory->AddDeleteFly(*pFormat, nChainInsPos);
                                // reset n so that no Format is skipped
                                n = n >= rSpzArr.size() ? rSpzArr.size() : n+1;
                            }
                            // Moving the anchor?
                            else if (!((DelContentType::CheckNoCntnt|DelContentType::ExcludeFlyAtStartEnd)
                                    & nDelContentType) &&
                                // for SwUndoDelete: rPoint is the node that
                                // will be Joined - so anchor should be moved
                                // off it - but UndoImpl() split will insert
                                // new node *before* existing one so a no-op
                                // may need to be done here to add it to
                                // history for Undo.
                                (rPoint.GetNodeIndex() == pAPos->GetNodeIndex()
                                 || pStt->GetNodeIndex() == pAPos->GetNodeIndex())
                                // Do not try to move the anchor to a table!
                                && rMark.GetNode().IsTextNode())
                            {
                                m_pHistory->AddChangeFlyAnchor(*pFormat);
                                SwFormatAnchor aAnch( *pAnchor );
                                SwPosition aPos( rMark.GetNode() );
                                aAnch.SetAnchor( &aPos );
                                pFormat->SetFormatAttr( aAnch );
                            }
                        }
                    }
                    break;
                case RndStdIds::FLY_AT_CHAR:
                    if( nullptr != (pAPos = pAnchor->GetContentAnchor() ) &&
                        ( pStt->GetNode() <= pAPos->GetNode() && pAPos->GetNode() <= pEnd->GetNode() ) )
                    {
                        if( !m_pHistory )
                            m_pHistory.reset( new SwHistory );
                        if (!(DelContentType::Replace & nDelContentType)
                            && IsDestroyFrameAnchoredAtChar(
                                *pAPos, *pStt, *pEnd, nDelContentType))
                        {
                            m_pHistory->AddDeleteFly(*pFormat, nChainInsPos);
                            n = n >= rSpzArr.size() ? rSpzArr.size() : n+1;
                        }
                        else if (!((DelContentType::CheckNoCntnt |
                                    DelContentType::ExcludeFlyAtStartEnd)
                                    & nDelContentType))
                        {
                            if( *pStt <= *pAPos && *pAPos < *pEnd )
                            {
                                // These are the objects anchored
                                // between section start and end position
                                // Do not try to move the anchor to a table!
                                if( rMark.GetNode().GetTextNode() )
                                {
                                    m_pHistory->AddChangeFlyAnchor(*pFormat);
                                    SwFormatAnchor aAnch( *pAnchor );
                                    aAnch.SetAnchor( &rMark );
                                    pFormat->SetFormatAttr( aAnch );
                                }
                            }
                        }
                    }
                    break;
                case RndStdIds::FLY_AT_FLY:

                    if( nullptr != (pAPos = pAnchor->GetContentAnchor() ) &&
                        pStt->GetNode() == pAPos->GetNode() )
                    {
                        if( !m_pHistory )
                            m_pHistory.reset( new SwHistory );

                        m_pHistory->AddDeleteFly(*pFormat, nChainInsPos);

                        // reset n so that no Format is skipped
                        n = n >= rSpzArr.size() ? rSpzArr.size() : n+1;
                    }
                    break;
                default: break;
                }
            }
        }
    }

    // 3. Bookmarks
    if( !(DelContentType::Bkm & nDelContentType) )
        return;

    IDocumentMarkAccess* const pMarkAccess = rDoc.getIDocumentMarkAccess();
    if( !pMarkAccess->getAllMarksCount() )
        return;

    for( sal_Int32 n = 0; n < pMarkAccess->getAllMarksCount(); ++n )
    {
        // #i81002#
        bool bSavePos = false;
        bool bSaveOtherPos = false;
        bool bDelete = false;
        const ::sw::mark::IMark *const pBkmk = pMarkAccess->getAllMarksBegin()[n];
        auto const type(IDocumentMarkAccess::GetType(*pBkmk));

        if( DelContentType::CheckNoCntnt & nDelContentType )
        {
            if ( pStt->GetNode() <= pBkmk->GetMarkPos().GetNode()
                 && pBkmk->GetMarkPos().GetNode() < pEnd->GetNode() )
            {
                bSavePos = true;
            }
            if ( pBkmk->IsExpanded()
                 && pStt->GetNode() <= pBkmk->GetOtherMarkPos().GetNode()
                 && pBkmk->GetOtherMarkPos().GetNode() < pEnd->GetNode() )
            {
                bSaveOtherPos = true;
            }
            bDelete = bSavePos && bSaveOtherPos;
        }
        else
        {
            // #i92125#
            // keep cross-reference bookmarks, if content inside one paragraph is deleted.
            if ( rMark.GetNode() == rPoint.GetNode()
                && (   type == IDocumentMarkAccess::MarkType::CROSSREF_HEADING_BOOKMARK
                    || type == IDocumentMarkAccess::MarkType::CROSSREF_NUMITEM_BOOKMARK))
            {
                continue;
            }

            bool bMaybe = false;
            if ( *pStt <= pBkmk->GetMarkPos() && pBkmk->GetMarkPos() <= *pEnd )
            {
                if ( pBkmk->GetMarkPos() == *pEnd
                     || ( *pStt == pBkmk->GetMarkPos() && pBkmk->IsExpanded() ) )
                    bMaybe = true;
                else
                    bSavePos = true;
            }
            if( pBkmk->IsExpanded() &&
                *pStt <= pBkmk->GetOtherMarkPos() && pBkmk->GetOtherMarkPos() <= *pEnd )
            {
                assert(!bSaveOtherPos);
                if (   bSavePos
                    || (*pStt < pBkmk->GetOtherMarkPos() && pBkmk->GetOtherMarkPos() < *pEnd)
                    || (bMaybe
                        && (   type == IDocumentMarkAccess::MarkType::TEXT_FIELDMARK
                            || type == IDocumentMarkAccess::MarkType::CHECKBOX_FIELDMARK
                            || type == IDocumentMarkAccess::MarkType::DROPDOWN_FIELDMARK
                            || type == IDocumentMarkAccess::MarkType::DATE_FIELDMARK))
                    || (bMaybe
                        && !(nDelContentType & DelContentType::Replace)
                        && type == IDocumentMarkAccess::MarkType::BOOKMARK
                        && pStt->GetContentIndex() == 0 // entire paragraph deleted?
                        && pEnd->GetContentIndex() == pEnd->GetNode().GetTextNode()->Len()))
                {
                    if( bMaybe )
                        bSavePos = true;
                    bDelete = true;
                }
                if (bDelete || pBkmk->GetOtherMarkPos() == *pEnd)
                {
                    bSaveOtherPos = true; // tdf#148389 always undo if at end
                }
            }
            if (!bSavePos && bMaybe && pBkmk->IsExpanded() && *pStt == pBkmk->GetMarkPos())
            {
                bSavePos = true; // tdf#148389 always undo if at start
            }

            if ( !bSavePos && !bSaveOtherPos
                 && dynamic_cast< const ::sw::mark::CrossRefBookmark* >(pBkmk) )
            {
                // certain special handling for cross-reference bookmarks
                const bool bDifferentTextNodesAtMarkAndPoint =
                    rMark.GetNode() != rPoint.GetNode()
                    && rMark.GetNode().GetTextNode()
                    && rPoint.GetNode().GetTextNode();
                if ( bDifferentTextNodesAtMarkAndPoint )
                {
                    // delete cross-reference bookmark at <pStt>, if only part of
                    // <pEnd> text node content is deleted.
                    if( pStt->GetNode() == pBkmk->GetMarkPos().GetNode()
                        && pEnd->GetContentIndex() != pEnd->GetNode().GetTextNode()->Len() )
                    {
                        bSavePos = true;
                        bSaveOtherPos = false; // cross-reference bookmarks are not expanded
                    }
                    // delete cross-reference bookmark at <pEnd>, if only part of
                    // <pStt> text node content is deleted.
                    else if( pEnd->GetNode() == pBkmk->GetMarkPos().GetNode() &&
                        pStt->GetContentIndex() != 0 )
                    {
                        bSavePos = true;
                        bSaveOtherPos = false; // cross-reference bookmarks are not expanded
                    }
                }
            }
            else if (type == IDocumentMarkAccess::MarkType::ANNOTATIONMARK)
            {
                // delete annotation marks, if its end position is covered by the deletion
                const SwPosition& rAnnotationEndPos = pBkmk->GetMarkEnd();
                if ( *pStt < rAnnotationEndPos && rAnnotationEndPos <= *pEnd )
                {
                    bSavePos = true;
                    bSaveOtherPos = pBkmk->IsExpanded(); //tdf#90138, only save the other pos if there is one
                    bDelete = true;
                }
            }
        }

        if ( bSavePos || bSaveOtherPos )
        {
            if (type != IDocumentMarkAccess::MarkType::UNO_BOOKMARK)
            {
                if( !m_pHistory )
                    m_pHistory.reset( new SwHistory );
                m_pHistory->AddIMark(*pBkmk, bSavePos, bSaveOtherPos);
            }
            if ( bSavePos
                 && (bDelete || !pBkmk->IsExpanded()))
            {
                pMarkAccess->deleteMark(pMarkAccess->getAllMarksBegin()+n, false);
                n--;
            }
        }
    }
}

// save a complete section into UndoNodes array
SwUndoSaveSection::SwUndoSaveSection()
    : m_nMoveLen( 0 ), m_nStartPos( NODE_OFFSET_MAX )
{
}

SwUndoSaveSection::~SwUndoSaveSection()
{
    if (m_oMovedStart) // delete also the section from UndoNodes array
    {
        // SaveSection saves the content in the PostIt section.
        SwNodes& rUNds = m_oMovedStart->GetNode().GetNodes();
        // cid#1486004 Uncaught exception
        suppress_fun_call_w_exception(rUNds.Delete(*m_oMovedStart, m_nMoveLen));

        m_oMovedStart.reset();
    }
    m_pRedlineSaveData.reset();
}

void SwUndoSaveSection::SaveSection( const SwNodeIndex& rSttIdx )
{
    SwNodeRange aRg( rSttIdx.GetNode(), *rSttIdx.GetNode().EndOfSectionNode() );
    SaveSection( aRg );
}

void SwUndoSaveSection::SaveSection(
    const SwNodeRange& rRange, bool const bExpandNodes)
{
    SwPaM aPam( rRange.aStart, rRange.aEnd );

    // delete all footnotes, fly frames, bookmarks
    DelContentIndex( *aPam.GetMark(), *aPam.GetPoint() );

    // redlines *before* CorrAbs, because DelBookmarks will make them 0-length
    // but *after* DelContentIndex because that also may use FillSaveData (in
    // flys) and that will be restored *after* this one...
    m_pRedlineSaveData.reset( new SwRedlineSaveDatas );
    if (!SwUndo::FillSaveData( aPam, *m_pRedlineSaveData ))
    {
        m_pRedlineSaveData.reset();
    }

    {
        // move certain indexes out of deleted range
        SwNodeIndex aSttIdx( aPam.Start()->GetNode() );
        SwNodeIndex aEndIdx( aPam.End()->GetNode() );
        SwNodeIndex aMvStt( aEndIdx, 1 );
        SwDoc::CorrAbs( aSttIdx, aEndIdx, SwPosition( aMvStt ), true );
    }

    m_nStartPos = rRange.aStart.GetIndex();

    if (bExpandNodes)
    {
        aPam.GetPoint()->Adjust(SwNodeOffset(-1));
        aPam.GetMark()->Adjust(SwNodeOffset(+1));
    }

    SwContentNode* pCNd = aPam.GetMarkContentNode();
    if( pCNd )
        aPam.GetMark()->SetContent( 0 );
    pCNd = aPam.GetPointContentNode();
    if( nullptr != pCNd )
        aPam.GetPoint()->SetContent( pCNd->Len() );

    // Keep positions as SwContentIndex so that this section can be deleted in DTOR
    SwNodeOffset nEnd;
    m_oMovedStart = rRange.aStart;
    MoveToUndoNds(aPam, &*m_oMovedStart, &nEnd);
    m_nMoveLen = nEnd - m_oMovedStart->GetIndex() + 1;
}

void SwUndoSaveSection::RestoreSection( SwDoc* pDoc, SwNodeIndex* pIdx,
                                        sal_uInt16 nSectType )
{
    if( NODE_OFFSET_MAX == m_nStartPos )        // was there any content?
        return;

    // check if the content is at the old position
    SwNodeIndex aSttIdx( pDoc->GetNodes(), m_nStartPos );

    // move the content from UndoNodes array into Fly
    SwStartNode* pSttNd = SwNodes::MakeEmptySection( aSttIdx.GetNode(),
                                            static_cast<SwStartNodeType>(nSectType) );

    RestoreSection( pDoc, *pSttNd->EndOfSectionNode() );

    if( pIdx )
        *pIdx = *pSttNd;
}

void SwUndoSaveSection::RestoreSection(
        SwDoc *const pDoc, const SwNode& rInsPos, bool bForceCreateFrames)
{
    if( NODE_OFFSET_MAX == m_nStartPos )        // was there any content?
        return;

    SwPosition aInsPos( rInsPos );
    SwNodeOffset nEnd = m_oMovedStart->GetIndex() + m_nMoveLen - 1;
    MoveFromUndoNds(*pDoc, m_oMovedStart->GetIndex(), aInsPos, &nEnd, bForceCreateFrames);

    // destroy indices again, content was deleted from UndoNodes array
    m_oMovedStart.reset();
    m_nMoveLen = SwNodeOffset(0);

    if( m_pRedlineSaveData )
    {
        SwUndo::SetSaveData( *pDoc, *m_pRedlineSaveData );
        m_pRedlineSaveData.reset();
    }
}

void SwUndoSaveSection::dumpAsXml(xmlTextWriterPtr pWriter) const
{
    SwUndoSaveContent::dumpAsXml(pWriter);
}

// save and set the RedlineData
SwRedlineSaveData::SwRedlineSaveData(
    SwComparePosition eCmpPos,
    const SwPosition& rSttPos,
    const SwPosition& rEndPos,
    SwRangeRedline& rRedl,
    bool bCopyNext )
    : SwUndRng( rRedl )
    , SwRedlineData( rRedl.GetRedlineData(), bCopyNext )
{
    assert( SwComparePosition::Outside == eCmpPos ||
            !rRedl.GetContentIdx() ); // "Redline with Content"

    switch (eCmpPos)
    {
    case SwComparePosition::OverlapBefore:        // Pos1 overlaps Pos2 at the beginning
        m_nEndNode = rEndPos.GetNodeIndex();
        m_nEndContent = rEndPos.GetContentIndex();
        break;

    case SwComparePosition::OverlapBehind:        // Pos1 overlaps Pos2 at the end
        m_nSttNode = rSttPos.GetNodeIndex();
        m_nSttContent = rSttPos.GetContentIndex();
        break;

    case SwComparePosition::Inside:                // Pos1 lays completely in Pos2
        m_nSttNode = rSttPos.GetNodeIndex();
        m_nSttContent = rSttPos.GetContentIndex();
        m_nEndNode = rEndPos.GetNodeIndex();
        m_nEndContent = rEndPos.GetContentIndex();
        break;

    case SwComparePosition::Outside:               // Pos2 lays completely in Pos1
        if ( rRedl.GetContentIdx() )
        {
            // than move section into UndoArray and memorize it
            SaveSection( *rRedl.GetContentIdx() );
            rRedl.ClearContentIdx();
        }
        break;

    case SwComparePosition::Equal:                 // Pos1 is exactly as big as Pos2
        break;

    default:
        assert(false);
    }

#if OSL_DEBUG_LEVEL > 0
    m_nRedlineCount = rSttPos.GetNode().GetDoc().getIDocumentRedlineAccess().GetRedlineTable().size();
    m_bRedlineCountDontCheck = false;
    m_bRedlineMoved = rRedl.IsMoved();
#endif
}

SwRedlineSaveData::~SwRedlineSaveData()
{
}

void SwRedlineSaveData::RedlineToDoc( SwPaM const & rPam )
{
    SwDoc& rDoc = rPam.GetDoc();
    SwRangeRedline* pRedl = new SwRangeRedline( *this, rPam );

    if( GetMvSttIdx() )
    {
        SwNodeIndex aIdx( rDoc.GetNodes() );
        RestoreSection( &rDoc, &aIdx, SwNormalStartNode );
        if( GetHistory() )
            GetHistory()->Rollback( &rDoc );
        pRedl->SetContentIdx( aIdx );
    }
    SetPaM( *pRedl );
    // First, delete the "old" so that in an Append no unexpected things will
    // happen, e.g. a delete in an insert. In the latter case the just restored
    // content will be deleted and not the one you originally wanted.
    rDoc.getIDocumentRedlineAccess().DeleteRedline( *pRedl, false, RedlineType::Any );

    RedlineFlags eOld = rDoc.getIDocumentRedlineAccess().GetRedlineFlags();
    rDoc.getIDocumentRedlineAccess().SetRedlineFlags_intern( eOld | RedlineFlags::DontCombineRedlines );
    //#i92154# let UI know about a new redline with comment
    if (rDoc.GetDocShell() && (!pRedl->GetComment().isEmpty()) )
        rDoc.GetDocShell()->Broadcast(SwRedlineHint());

    auto const result(rDoc.getIDocumentRedlineAccess().AppendRedline(pRedl, true));
    assert(result != IDocumentRedlineAccess::AppendResult::IGNORED); // SwRedlineSaveData::RedlineToDoc: insert redline failed
    (void) result; // unused in non-debug
    rDoc.getIDocumentRedlineAccess().SetRedlineFlags_intern( eOld );
}

bool SwUndo::FillSaveData(
    const SwPaM& rRange,
    SwRedlineSaveDatas& rSData,
    bool bDelRange,
    bool bCopyNext )
{
    rSData.clear();

    auto [pStt, pEnd] = rRange.StartEnd(); // SwPosition*
    const SwRedlineTable& rTable = rRange.GetDoc().getIDocumentRedlineAccess().GetRedlineTable();
    SwRedlineTable::size_type n = 0;
    rRange.GetDoc().getIDocumentRedlineAccess().GetRedline( *pStt, &n );
    for ( ; n < rTable.size(); ++n )
    {
        SwRangeRedline* pRedl = rTable[n];

        const SwComparePosition eCmpPos =
            ComparePosition( *pStt, *pEnd, *pRedl->Start(), *pRedl->End() );
        if ( eCmpPos != SwComparePosition::Before
             && eCmpPos != SwComparePosition::Behind
             && eCmpPos != SwComparePosition::CollideEnd
             && eCmpPos != SwComparePosition::CollideStart )
        {

            rSData.push_back(std::unique_ptr<SwRedlineSaveData>(new SwRedlineSaveData(eCmpPos, *pStt, *pEnd, *pRedl, bCopyNext)));
        }
    }
    if( !rSData.empty() && bDelRange )
    {
        rRange.GetDoc().getIDocumentRedlineAccess().DeleteRedline( rRange, false, RedlineType::Any );
    }
    return !rSData.empty();
}

bool SwUndo::FillSaveDataForFormat(
    const SwPaM& rRange,
    SwRedlineSaveDatas& rSData )
{
    rSData.clear();

    const SwPosition *pStt = rRange.Start(), *pEnd = rRange.End();
    const SwRedlineTable& rTable = rRange.GetDoc().getIDocumentRedlineAccess().GetRedlineTable();
    SwRedlineTable::size_type n = 0;
    rRange.GetDoc().getIDocumentRedlineAccess().GetRedline( *pStt, &n );
    for ( ; n < rTable.size(); ++n )
    {
        SwRangeRedline* pRedl = rTable[n];
        if ( RedlineType::Format == pRedl->GetType() )
        {
            const SwComparePosition eCmpPos = ComparePosition( *pStt, *pEnd, *pRedl->Start(), *pRedl->End() );
            if ( eCmpPos != SwComparePosition::Before
                 && eCmpPos != SwComparePosition::Behind
                 && eCmpPos != SwComparePosition::CollideEnd
                 && eCmpPos != SwComparePosition::CollideStart )
            {
                rSData.push_back(std::unique_ptr<SwRedlineSaveData>(new SwRedlineSaveData(eCmpPos, *pStt, *pEnd, *pRedl, true)));
            }

        }
    }
    return !rSData.empty();
}


void SwUndo::SetSaveData( SwDoc& rDoc, SwRedlineSaveDatas& rSData )
{
    RedlineFlags eOld = rDoc.getIDocumentRedlineAccess().GetRedlineFlags();
    rDoc.getIDocumentRedlineAccess().SetRedlineFlags_intern( ( eOld & ~RedlineFlags::Ignore) | RedlineFlags::On );
    SwPaM aPam( rDoc.GetNodes().GetEndOfContent() );

    for( size_t n = rSData.size(); n; )
        rSData[ --n ].RedlineToDoc( aPam );

#if OSL_DEBUG_LEVEL > 0
    // check redline count against count saved in RedlineSaveData object
    // except in the case of moved redlines
    assert(
        rSData.empty() || rSData[0].m_bRedlineMoved || rSData[0].m_bRedlineCountDontCheck ||
           (rSData[0].m_nRedlineCount == rDoc.getIDocumentRedlineAccess().GetRedlineTable().size()));
            // "redline count not restored properly"
#endif

    rDoc.getIDocumentRedlineAccess().SetRedlineFlags_intern( eOld );
}

bool SwUndo::HasHiddenRedlines( const SwRedlineSaveDatas& rSData )
{
    for( size_t n = rSData.size(); n; )
        if( rSData[ --n ].GetMvSttIdx() )
            return true;
    return false;
}

bool SwUndo::CanRedlineGroup( SwRedlineSaveDatas& rCurr,
                        const SwRedlineSaveDatas& rCheck, bool bCurrIsEnd )
{
    if( rCurr.size() != rCheck.size() )
        return false;

    for( size_t n = 0; n < rCurr.size(); ++n )
    {
        const SwRedlineSaveData& rSet = rCurr[ n ];
        const SwRedlineSaveData& rGet = rCheck[ n ];
        if( rSet.m_nSttNode != rGet.m_nSttNode ||
            rSet.GetMvSttIdx() || rGet.GetMvSttIdx() ||
            ( bCurrIsEnd ? rSet.m_nSttContent != rGet.m_nEndContent
                            : rSet.m_nEndContent != rGet.m_nSttContent ) ||
            !rGet.CanCombine( rSet ) )
        {
            return false;
        }
    }

    for( size_t n = 0; n < rCurr.size(); ++n )
    {
        SwRedlineSaveData& rSet = rCurr[ n ];
        const SwRedlineSaveData& rGet = rCheck[ n ];
        if( bCurrIsEnd )
            rSet.m_nSttContent = rGet.m_nSttContent;
        else
            rSet.m_nEndContent = rGet.m_nEndContent;
    }
    return true;
}

OUString ShortenString(const OUString & rStr, sal_Int32 nLength, std::u16string_view aFillStr)
{
    assert(nLength - aFillStr.size() >= 2);

    if (rStr.getLength() <= nLength)
        return rStr;

    nLength -= aFillStr.size();
    if ( nLength < 2 )
        nLength = 2;

    const sal_Int32 nFrontLen = nLength - nLength / 2;
    const sal_Int32 nBackLen = nLength - nFrontLen;

    return OUString::Concat(rStr.subView(0, nFrontLen))
           + aFillStr
           + rStr.subView(rStr.getLength() - nBackLen);
}

static bool IsAtEndOfSection(SwPosition const& rAnchorPos)
{
    SwNodeIndex node(*rAnchorPos.GetNode().EndOfSectionNode());
    SwContentNode *const pNode(SwNodes::GoPrevious(&node));
    assert(pNode);
    assert(rAnchorPos.GetNode() <= node.GetNode()); // last valid anchor pos is last content
    return node == rAnchorPos.GetNode()
        // at-para fly has no SwContentIndex!
        && (rAnchorPos.GetContentIndex() == pNode->Len() || rAnchorPos.GetContentNode() == nullptr);
}

static bool IsAtStartOfSection(SwPosition const& rAnchorPos)
{
    SwNodes const& rNodes(rAnchorPos.GetNodes());
    SwNodeIndex node(*rAnchorPos.GetNode().StartOfSectionNode());
    SwContentNode *const pNode(rNodes.GoNext(&node));
    assert(pNode);
    (void) pNode;
    assert(node <= rAnchorPos.GetNode());
    return node == rAnchorPos.GetNode() && rAnchorPos.GetContentIndex() == 0;
}

/// passed start / end position could be on section start / end node
static bool IsAtEndOfSection2(SwPosition const& rPos)
{
    return rPos.GetNode().IsEndNode()
        || IsAtEndOfSection(rPos);
}

static bool IsAtStartOfSection2(SwPosition const& rPos)
{
    return rPos.GetNode().IsStartNode()
        || IsAtStartOfSection(rPos);
}

static bool IsNotBackspaceHeuristic(
        SwPosition const& rStart, SwPosition const& rEnd)
{
    // check if the selection is backspace/delete created by DelLeft/DelRight
    if (rStart.GetNodeIndex() + 1 != rEnd.GetNodeIndex())
        return true;
    if (rEnd.GetContentIndex() != 0)
        return true;
    const SwTextNode* pTextNode = rStart.GetNode().GetTextNode();
    if (!pTextNode || rStart.GetContentIndex() != pTextNode->Len())
        return true;
    return false;
}

bool IsDestroyFrameAnchoredAtChar(SwPosition const & rAnchorPos,
        SwPosition const & rStart, SwPosition const & rEnd,
        DelContentType const nDelContentType)
{
    assert(rStart <= rEnd);

    // CheckNoCntnt means DelFullPara which is obvious to handle
    if (DelContentType::CheckNoCntnt & nDelContentType)
    {   // exclude selection end node because it won't be deleted
        return (rAnchorPos.GetNode() < rEnd.GetNode())
            && (rStart.GetNode() <= rAnchorPos.GetNode());
    }

    if ((nDelContentType & DelContentType::WriterfilterHack)
        && rAnchorPos.GetDoc().IsInWriterfilterImport())
    {   // FIXME hack for writerfilter RemoveLastParagraph() and MakeFlyAndMove(); can't test file format more specific?
        return (rStart < rAnchorPos) && (rAnchorPos < rEnd);
    }

    if (nDelContentType & DelContentType::ExcludeFlyAtStartEnd)
    {   // exclude selection start and end node
        return (rAnchorPos.GetNode() < rEnd.GetNode())
            && (rStart.GetNode() < rAnchorPos.GetNode());
    }

    // in general, exclude the start and end position
    return ((rStart < rAnchorPos)
            || (rStart == rAnchorPos
                // special case: fully deleted node
                && ((rStart.GetNode() != rEnd.GetNode() && rStart.GetContentIndex() == 0
                        // but not if the selection is backspace/delete!
                        && IsNotBackspaceHeuristic(rStart, rEnd))
                    || (IsAtStartOfSection(rAnchorPos) && IsAtEndOfSection2(rEnd)))))
        && ((rAnchorPos < rEnd)
            || (rAnchorPos == rEnd
                // special case: fully deleted node
                && ((rEnd.GetNode() != rStart.GetNode() && rEnd.GetContentIndex() == rEnd.GetNode().GetTextNode()->Len()
                        && IsNotBackspaceHeuristic(rStart, rEnd))
                    || (IsAtEndOfSection(rAnchorPos) && IsAtStartOfSection2(rStart)))));
}

bool IsSelectFrameAnchoredAtPara(SwPosition const & rAnchorPos,
        SwPosition const & rStart, SwPosition const & rEnd,
        DelContentType const nDelContentType)
{
    assert(rStart <= rEnd);

    // CheckNoCntnt means DelFullPara which is obvious to handle
    if (DelContentType::CheckNoCntnt & nDelContentType)
    {   // exclude selection end node because it won't be deleted
        return (rAnchorPos.GetNode() < rEnd.GetNode())
            && (rStart.GetNode() <= rAnchorPos.GetNode());
    }

    if ((nDelContentType & DelContentType::WriterfilterHack)
        && rAnchorPos.GetDoc().IsInWriterfilterImport())
    {   // FIXME hack for writerfilter RemoveLastParagraph() and MakeFlyAndMove(); can't test file format more specific?
        // but it MUST NOT be done during the SetRedlineFlags at the end of ODF
        // import, where the IsInXMLImport() cannot be checked because the
        // stupid code temp. overrides it - instead rely on setting the ALLFLYS
        // flag in MoveFromSection() and converting that to CheckNoCntnt with
        // adjusted cursor!
        return (rStart.GetNode() < rAnchorPos.GetNode()) && (rAnchorPos.GetNode() < rEnd.GetNode());
    }

    // in general, exclude the start and end position
    return ((rStart.GetNode() < rAnchorPos.GetNode())
            || (rStart.GetNode() == rAnchorPos.GetNode()
                && !(nDelContentType & DelContentType::ExcludeFlyAtStartEnd)
                // special case: fully deleted node
                && ((rStart.GetNode() != rEnd.GetNode() && rStart.GetContentIndex() == 0
                        // but not if the selection is backspace/delete!
                        && IsNotBackspaceHeuristic(rStart, rEnd))
                    || (IsAtStartOfSection2(rStart) && IsAtEndOfSection2(rEnd)))))
        && ((rAnchorPos.GetNode() < rEnd.GetNode())
            || (rAnchorPos.GetNode() == rEnd.GetNode()
                && !(nDelContentType & DelContentType::ExcludeFlyAtStartEnd)
                // special case: fully deleted node
                && ((rEnd.GetNode() != rStart.GetNode() && rEnd.GetContentIndex() == rEnd.GetNode().GetTextNode()->Len()
                        && IsNotBackspaceHeuristic(rStart, rEnd))
                    || (IsAtEndOfSection2(rEnd) && IsAtStartOfSection2(rStart)))));
}

bool IsFlySelectedByCursor(SwDoc const & rDoc,
        SwPosition const & rStart, SwPosition const & rEnd)
{
    for (SwFrameFormat const*const pFly : *rDoc.GetSpzFrameFormats())
    {
        SwFormatAnchor const& rAnchor(pFly->GetAnchor());
        switch (rAnchor.GetAnchorId())
        {
            case RndStdIds::FLY_AT_CHAR:
            case RndStdIds::FLY_AT_PARA:
            {
                SwPosition const*const pAnchorPos(rAnchor.GetContentAnchor());
                // can this really be null?
                if (pAnchorPos != nullptr
                    && ((rAnchor.GetAnchorId() == RndStdIds::FLY_AT_CHAR)
                        ? IsDestroyFrameAnchoredAtChar(*pAnchorPos, rStart, rEnd)
                        : IsSelectFrameAnchoredAtPara(*pAnchorPos, rStart, rEnd)))
                {
                    return true;
                }
            }
            break;
            default: // other types not relevant
            break;
        }
    }
    return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
