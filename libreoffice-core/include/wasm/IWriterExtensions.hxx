#pragma once

#include <vcl/dllapi.h>
#include <emscripten/val.h>

namespace wasm
{
using namespace emscripten;
class VCL_DLLPUBLIC SAL_LOPLUGIN_ANNOTATE("crosscast") ITextRanges
{
public:
    virtual ~ITextRanges(){};
    virtual int length() { return 0; };
    virtual val rect(int /* index */) { return {}; };
    virtual val rects(int /* startYPosTwips */, int /* endYPosTwips */) { return {}; };
    virtual bool isCursorAt(int /* index */) { return false; };
    virtual int indexAtCursor() { return -1; };
    virtual void moveCursorTo(int /* index */, bool /* end */, bool /* select */) {};
    virtual val description(int /* index */) { return {}; };
    virtual val descriptions(int /* startIndex */, int /* endIndex */) { return {}; };
    virtual void replace(int /* index */, const std::string& /* text */) {};
    virtual void replaceAll(const std::string& /* text */) {};
};

class VCL_DLLPUBLIC SAL_LOPLUGIN_ANNOTATE("crosscast") IWriterExtensions
{
public:
    virtual val comments() { return {}; };
    virtual void addComment(const std::string& /* text */) {}
    virtual void replyComment(int /* parentId */, const std::string& /* text */) {};
    virtual void deleteCommentThreads(val /* parentId */){};
    virtual void deleteComment(int /* commentId */) {};
    virtual void resolveCommentThread(int /* parentId */) {};
    virtual void resolveComment(int /* commentId */) {};
    virtual void sanitize(val /* options */){};
    virtual val pageRects() { return {}; };
    virtual val headerFooterRect() { return {}; };
    virtual val paragraphStyles() { return {}; };
    virtual std::shared_ptr<ITextRanges> findAllTextRanges(const std::string& /* text */,
                                                           val /* flags */)
    {
        return {};
    };
    virtual void cancelFindOrReplace() {};
};
}
