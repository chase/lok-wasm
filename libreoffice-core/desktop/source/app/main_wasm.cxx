#define LOK_USE_UNSTABLE_API
#include <unordered_set>
#include <LibreOfficeKit/LibreOfficeKitEnums.h>
#include <LibreOfficeKit/LibreOfficeKit.hxx>
#include <emscripten.h>
#include <emscripten/em_asm.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/promise.h>
#include <array>
#include <cstdint>
#include <vector>
#include <memory>
#include <lib/wasm_extensions.hxx>
#include <sal/log.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Type.hxx>
#include <typelib/typedescription.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertyContainer.hpp>
#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/text/XTextRange.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/text/XTextViewCursorSupplier.hpp>
#include <com/sun/star/text/XTextViewCursor.hpp>
#include <com/sun/star/datatransfer/XTransferable.hpp>
#include <com/sun/star/text/XPageCursor.hdl>
#include <com/sun/star/style/XStyleFamiliesSupplier.hpp>
#include <com/sun/star/style/XStyle.hpp>
#include <com/sun/star/beans/XMultiPropertySet.hpp>
#include <wasm/IWriterExtensions.hxx>
#include <vcl/ITiledRenderable.hxx>
#include <rtl/string.hxx>
#include <rtl/ustring.hxx>
#include <o3tl/any.hxx>

namespace
{
using namespace emscripten;

//static
lok::Office* instance()
{
    static lok::Office* instance_ = nullptr;
    if (!instance_)
    {
        instance_ = lok::lok_cpp_init(/* use default path */ nullptr);
        instance_->setOptionalFeatures(
            LOK_FEATURE_PART_IN_INVALIDATION_CALLBACK | LOK_FEATURE_NO_TILED_ANNOTATIONS
            | LOK_FEATURE_RANGE_HEADERS | LOK_FEATURE_VIEWID_IN_VISCURSOR_INVALIDATION_CALLBACK);
    }
    return instance_;
}

//static
void preload() { instance(); }

static constexpr std::string_view TEXT_PLAIN = "text/plain";

uint32_t document_id_counter = 0;

// actually more unsafe than safe regarding memory safety, but it's safe to pass between workers
using SafeString = std::pair<size_t, char*>;
SafeString makeSafeString(const char* src)
{
    auto len = std::strlen(src) + 1; // +1 for the null terminator
    char* dst = new char[len];
    std::memcpy(dst, src, len);
    return std::make_pair((size_t)dst, dst);
}

void freeSafeString(size_t id) { delete[] (char*)id; }

val unoAnyToVal(const css::uno::Any& any);
void addStructToVal(typelib_CompoundTypeDescription* desc, void const* source, val& obj)
{
    if (desc->pBaseTypeDescription != nullptr)
    {
        addStructToVal(desc->pBaseTypeDescription, source, obj);
    }
    for (sal_Int32 i = 0; i != desc->nMembers; ++i)
    {
        css::uno::Any any_val(
            const_cast<char*>(static_cast<char const*>(source) + desc->pMemberOffsets[i]),
            desc->ppTypeRefs[i]);
        obj.set(val::u16string(desc->ppMemberNames[i]->buffer), unoAnyToVal(any_val));
    }
}

val unoAnyToVal(const css::uno::Any& any)
{
    using namespace css::uno;
    switch (any.getValueTypeClass())
    {
        case TypeClass::TypeClass_VOID:
            return val::null();
        case TypeClass::TypeClass_CHAR:
            return val(*static_cast<sal_Unicode const*>(any.getValue()));
        case TypeClass::TypeClass_BOOLEAN:
            return val(*o3tl::forceAccess<bool>(any));
        case TypeClass::TypeClass_BYTE:
        case TypeClass::TypeClass_SHORT:
        case TypeClass::TypeClass_LONG:
        case TypeClass::TypeClass_HYPER:
            return val(*o3tl::forceAccess<sal_Int32>(any));
        case TypeClass::TypeClass_UNSIGNED_LONG:
        case TypeClass::TypeClass_UNSIGNED_SHORT:
        case TypeClass::TypeClass_UNSIGNED_HYPER:
            return val(*o3tl::forceAccess<sal_uInt32>(any));
        case TypeClass::TypeClass_FLOAT:
        case TypeClass::TypeClass_DOUBLE:
            return val(*o3tl::forceAccess<double>(any));
        case TypeClass::TypeClass_STRING:
            return val::u16string(any.get<rtl::OUString>().getStr());
        case TypeClass::TypeClass_TYPE:
            return val(o3tl::forceAccess<Type>(any)->getTypeName());
        case TypeClass::TypeClass_ENUM:
            return val(*static_cast<sal_Int32 const*>(any.getValue()));
        case TypeClass::TypeClass_STRUCT:
        case TypeClass::TypeClass_EXCEPTION:
        {
            TypeDescription desc(any.getValueTypeRef());
            if (!desc.is())
            {
                EM_ASM({ console.error('invalid uno::struct'); });
                return val::null();
            }
            auto const td = reinterpret_cast<typelib_CompoundTypeDescription*>(desc.get());
            val obj = val::object();
            addStructToVal(td, any.getValue(), obj);
            return obj;
        }
        case TypeClass::TypeClass_TYPEDEF:
        case TypeClass::TypeClass_UNION:
        case TypeClass::TypeClass_SEQUENCE:
        case TypeClass::TypeClass_ARRAY:
        case TypeClass::TypeClass_INTERFACE:
        case TypeClass::TypeClass_SERVICE:
        case TypeClass::TypeClass_MODULE:
        case TypeClass::TypeClass_INTERFACE_METHOD:
        case TypeClass::TypeClass_INTERFACE_ATTRIBUTE:
        case TypeClass::TypeClass_UNKNOWN:
        case TypeClass::TypeClass_PROPERTY:
        case TypeClass::TypeClass_CONSTANT:
        case TypeClass::TypeClass_CONSTANTS:
        case TypeClass::TypeClass_SINGLETON:
        case TypeClass::TypeClass_MAKE_FIXED_SIZE:
        case TypeClass::TypeClass_ANY:
        {
            EM_ASM({ console.error('unsupported uno::any type'); });
            return val::null();
        }
    }
}

css::uno::Any valToUnoAny(const val& obj)
{
    using namespace css::uno;
    if (obj.isNull() || obj.isUndefined())
        return {};
    if (obj.isString())
        return Any(OUString::fromUtf8(std::move(obj.as<std::string>())));
    if (obj.isNumber())
        return Any(obj.as<double>());
    if (obj.isTrue())
        return Any(true);
    if (obj.isFalse())
        return Any(true);
    EM_ASM({ console.error('unsupported js to uno::any'); });
    return {};
}

css::uno::Sequence<rtl::OUString> valStrArrayToSequence(val v)
{
    using namespace css::uno;
    const uint32_t l = v["length"].as<uint32_t>();
    Sequence<rtl::OUString> rv(l);
    rtl::OUString* pArray = rv.getArray();

    for (uint32_t i = 0; i < l; ++i)
    {
        pArray[i] = OUString::fromUtf8(std::move(v[i].as<std::string>()));
    }

    return rv;
}

class DocumentClient final
{
private:
    desktop::WasmDocumentExtension* ext()
    {
        return static_cast<desktop::WasmDocumentExtension*>(doc_->get());
    }

    wasm::IWriterExtensions* writer()
    {
        return dynamic_cast<wasm::IWriterExtensions*>(ext()->mxComponent.get());
    }

    css::uno::Reference<css::container::XNameAccess> _paragraphStyles()
    {
        using namespace css;
        using namespace css::uno;
        Reference<style::XStyleFamiliesSupplier> xStyleFamiliesSupplier(ext()->mxComponent,
                                                                        UNO_QUERY_THROW);
        Reference<container::XNameAccess> xStyleFamilies
            = xStyleFamiliesSupplier->getStyleFamilies();

        if (!xStyleFamilies.is())
            return {};

        Reference<container::XNameAccess> xStyles(xStyleFamilies->getByName("ParagraphStyles"),
                                                  UNO_QUERY);
        return xStyles;
    }

public:
    explicit DocumentClient(std::string path)
        : ref_(++document_id_counter)
        , doc_(instance()->documentLoad(path.c_str()))
    {
    }
    bool valid() { return doc_ != nullptr; }

    bool saveAs(std::string url, std::optional<std::string> format,
                std::optional<std::string> filterOptions)
    {
        return doc_->saveAs(url.c_str(), format.has_value() ? format->c_str() : nullptr,
                            filterOptions.has_value() ? filterOptions->c_str() : nullptr);
    }

    int getParts() { return doc_->getParts(); }
    val getPartRectangles() { return val::u8string(doc_->getPartPageRectangles()); }
    val paintTile(int tileWidth, int tileHeight, int xTwips, int yTwips, int widthTwips,
                  int heightTwips)
    {
        doc_->paintTile(tile_scratch_.data(), tileWidth, tileHeight, xTwips, yTwips, widthTwips,
                        heightTwips);
        return val(
            typed_memory_view(tileWidth * tileHeight * BYTES_PER_PIXEL, tile_scratch_.data()));
    }

    val getDocumentSize()
    {
        long size[2];
        val result = val::array();
        doc_->getDocumentSize(&size[0], &size[1]);
        result.call<void>("push", val(size[0]));
        result.call<void>("push", val(size[1]));
        return result;
    }

    void initializeForRendering(std::string args)
    {
        rendering_tiles_ = true;
        doc_->initializeForRendering(args.c_str());
        registerCallbackOnce(doc_->getView());
    }

    void postKeyEvent(int viewId, int type, int charCode, int keyCode)
    {
        doc_->setView(viewId);
        doc_->postKeyEvent(type, charCode, keyCode);
    }

    void postTextInputEvent(int viewId, unsigned windowId, std::string text)
    {
        doc_->setView(viewId);
        doc_->postWindowExtTextInputEvent(windowId, LOK_EXT_TEXTINPUT, text.c_str());
        doc_->postWindowExtTextInputEvent(windowId, LOK_EXT_TEXTINPUT_END, text.c_str());
    }

    void postMouseEvent(int viewId, int type, int x, int y, int count, int buttons, int modifier)
    {
        doc_->setView(viewId);
        doc_->postMouseEvent(type, x, y, count, buttons, modifier);
    }

    void postUnoCommand(int viewId, std::string command, std::optional<std::string> args,
                        bool notifyWhenFinished)
    {
        doc_->setView(viewId);
        doc_->postUnoCommand(command.c_str(), args.has_value() ? args->c_str() : nullptr,
                             notifyWhenFinished);
    }

    void setTextSelection(int viewId, int type, int x, int y)
    {
        doc_->setView(viewId);
        doc_->setTextSelection(type, x, y);
    }

    bool setClipboard(int viewId, val data)
    {
        if (!data.isArray())
            return false;
        const uint32_t entries = data["length"].as<uint32_t>();
        if (entries == 0)
            return false;

        std::vector<const char*> mime_c_str;
        std::vector<size_t> in_sizes;
        in_sizes.reserve(entries);
        std::vector<const char*> streams;
        streams.reserve(entries);

        for (size_t i = 0; i < entries; ++i)
        {
            if (!data[i]["mimeType"].isString())
                continue;

            std::string mime_type = data[i]["mimeType"].as<std::string>();
            std::string buffer = data[i]["buffer"].as<std::string>();

            in_sizes[i] = buffer.size();
            mime_c_str.push_back(mime_type.c_str());
            streams[i] = buffer.data();
        }

        // add the nullptr terminator to the list of null-terminated strings
        mime_c_str.push_back(nullptr);

        doc_->setView(viewId);
        return doc_->setClipboard(entries, mime_c_str.data(), in_sizes.data(), streams.data());
    }

    val getClipboard(int viewId, val mimeTypes)
    {
        std::vector<const char*> mime_c_str;
        if (mimeTypes.isArray())
        {
            const uint32_t len = mimeTypes["length"].as<uint32_t>();
            mime_c_str.reserve(len + 1); // +1 is for the list terminator
            for (uint32_t i = 0; i < len; ++i)
            {
                std::string mime = mimeTypes[i].as<std::string>();
                // LOK explicitly converts all UTF-16 strings to UTF-8, however it still
                // requests an encoding
                if (mime == TEXT_PLAIN)
                {
                    mime_c_str.push_back("text/plain;charset=utf-8");
                    continue;
                }
                else
                {
                    mime_c_str.push_back(mime.c_str());
                }
            }
            // add the nullptr terminator to the list of null-terminated strings
            mime_c_str.push_back(nullptr);
        }

        size_t out_count;
        // these are arrays of out_count size, variable size arrays in C are
        // simply pointers to the first element
        char** out_mime_types = nullptr;
        size_t* out_sizes = nullptr;
        char** out_streams = nullptr;

        doc_->setView(viewId);
        bool success = doc_->getClipboard(mime_c_str.empty() ? nullptr : mime_c_str.data(),
                                          &out_count, &out_mime_types, &out_sizes, &out_streams);
        val result = val::array();
        if (!success)
            return result;

        for (size_t i = 0; i < out_count; ++i)
        {
            size_t buffer_size = out_sizes[i];
            if (buffer_size <= 0)
            {
                result.set(i, val::undefined());
                continue;
            }
            static constexpr std::string_view text_prefix = "text/";
            std::string_view sv_mime_type(out_mime_types[i]);
            val obj = val::object();
            if (sv_mime_type.substr(0, text_prefix.length()) == text_prefix)
            {
                // LOK likes to add character encodings and other odd bits, but browsers expect `text/plain`
                obj.set("mimeType",
                        val::u8string(sv_mime_type.substr(0, TEXT_PLAIN.length()) == TEXT_PLAIN
                                          ? TEXT_PLAIN.data()
                                          : out_mime_types[i]));
                obj.set("text", val::u8string(out_streams[i]));
            }
            else
            {
                obj.set("mimeType", out_mime_types[i]);
                obj.set("data", typed_memory_view(out_sizes[i], out_streams[i]));
            }
            result.call<void>("push", obj);

            // free the clipboard item, can't use std::unique_ptr without a
            // wrapper class since it needs to be size aware
            free(out_streams[i]);
            free(out_mime_types[i]);
        }
        // free the clipboard item containers
        free(out_sizes);
        free(out_streams);
        free(out_mime_types);

        return result;
    }

    void paste(int viewId, std::string mimeType, std::string data)
    {
        doc_->setView(viewId);
        doc_->paste(mimeType.c_str(), data.c_str(), data.size());
    }

    void setGraphicSelection(int viewId, int type, int x, int y)
    {
        doc_->setView(viewId);
        doc_->setGraphicSelection(type, x, y);
    }
    void resetSelection(int viewId)
    {
        doc_->setView(viewId);
        doc_->resetSelection();
    }

    val getCommandValues(int viewId, std::string command)
    {
        doc_->setView(viewId);
        desktop::WasmDocumentExtension* ext
            = static_cast<desktop::WasmDocumentExtension*>(doc_->get());

        if (command == ".uno:PageColor")
        {
            return val(ext->getPageColor());
        }
        else if (command == ".uno:PageOrientation")
        {
            return val(ext->getPageOrientation());
        }
        return val::u8string(doc_->getCommandValues(command.c_str()));
    }

    void subscribe(int viewId, int type)
    {
        registerCallbackOnce(viewId);
        subscribed_events_[viewId].emplace(type);
    }

    void unsubscribe(int viewId, int type) { subscribed_events_[viewId].erase(type); }

    uint32_t ref() const { return ref_; }

    int32_t getViewId() { return doc_->getView(); }

    int32_t newView() { return doc_->createView(); }

    val startTileRenderer(int32_t viewId, int32_t tileSize)
    {
        desktop::TileRendererData& data = ext()->startTileRenderer(viewId, tileSize);
        val result = val::object();
        result.set("viewId", data.viewId);
        result.set("tileSize", data.tileSize);
        result.set("state", typed_memory_view(1, (int32_t*)&data.state));
        result.set("tileTwips", typed_memory_view(4, (uint32_t*)&data.tileTwips));
        result.set("paintedTile", typed_memory_view(data.paintedTileAllocSize, data.paintedTile));
        result.set("pendingFullPaint", typed_memory_view(1, (int32_t*)&data.pendingFullPaint));
        result.set("hasInvalidations", typed_memory_view(1, (int32_t*)&data.hasInvalidations));
        result.set("invalidationStack", typed_memory_view(4 * desktop::MAX_INVALIDATION_STACK,
                                                          (uint32_t*)&data.invalidationStack));
        result.set("invalidationStackHead",
                   typed_memory_view(1, (int32_t*)&data.invalidationStackHead));
        result.set("docWidthTwips", typed_memory_view(1, (uint32_t*)&data.docWidthTwips));
        result.set("docHeightTwips", typed_memory_view(1, (uint32_t*)&data.docHeightTwips));

        return result;
    }

    void stopTileRenderer(int32_t viewId)
    {
        ext()->stopTileRenderer(viewId);
    }

    void dispatchCommand(int viewId, std::string command, std::optional<std::string> arguments,
                         std::optional<bool> notifyWhenFinished)
    {
        doc_->setView(viewId);
        doc_->postUnoCommand(command.c_str(), arguments.has_value() ? arguments->c_str() : nullptr,
                             notifyWhenFinished.value_or(false));
    }

    void removeText(int viewId, unsigned int windowId, int charsBefore, int charsAfter)
    {
        doc_->setView(viewId);
        doc_->removeTextContext(windowId, charsBefore, charsAfter);
    }

    void setCurrentView(int viewId) { doc_->setView(viewId); }

    // NOTE: Disabled until unoembind startup cost is under 1s
    /*
    css::uno::Reference<css::lang::XComponent> getXComponent(int viewId)
    {
        doc_->setView(viewId);
        return static_cast<desktop::WasmDocumentExtension*>(doc_->get())->mxComponent;
    }
    */

    void setClientVisibleArea(int viewId, int x, int y, int width, int height)
    {
        doc_->setView(viewId);
        doc_->setClientVisibleArea(x, y, width, height);
    }

    val getPropertyValue(std::string property)
    {
        using namespace css;
        using namespace css::uno;
        Reference<beans::XPropertySet> xProp(ext()->mxComponent, UNO_QUERY_THROW);

        return unoAnyToVal(xProp->getPropertyValue(OUString::fromUtf8(property)));
    }

    void setPropertyValue(std::string property, val value)
    {
        using namespace css;
        using namespace css::uno;
        Reference<beans::XPropertySet> xProp(ext()->mxComponent, UNO_QUERY_THROW);

        return xProp->setPropertyValue(OUString::fromUtf8(property), valToUnoAny(value));
    }

    void saveCurrentSelection()
    {
        using namespace css;
        using namespace css::uno;
        Reference<frame::XModel> xModel(ext()->mxComponent, UNO_QUERY_THROW);
        Reference<container::XIndexAccess> xSelections(xModel->getCurrentSelection(),
                                                       uno::UNO_QUERY);
        if (!xSelections.is() || xSelections->getCount() < 1)
        {
            stored_range_.clear();
            return;
        }
        Reference<text::XTextRange> xSelection(xSelections->getByIndex(0), uno::UNO_QUERY);
        stored_range_ = xSelection;
    }

    void restoreCurrentSelection()
    {
        using namespace css;
        using namespace css::uno;
        if (!stored_range_.is())
            return;

        Reference<frame::XModel> xModel(ext()->mxComponent, UNO_QUERY_THROW);
        Reference<text::XTextViewCursorSupplier> xCursorSupplier(xModel->getCurrentController(),
                                                                 UNO_QUERY_THROW);
        Reference<text::XTextViewCursor> xCursor = xCursorSupplier->getViewCursor();
        xCursor->gotoRange(stored_range_, false);
        // xCursor->gotoRange(stored_range_, true); // TODO: necessary?
    }

    val getSelectionText()
    {
        using namespace css;
        using namespace css::uno;
        Reference<frame::XModel> xModel(ext()->mxComponent, UNO_QUERY_THROW);
        Reference<text::XTextViewCursorSupplier> xCursorSupplier(xModel->getCurrentController(),
                                                                 UNO_QUERY_THROW);
        Reference<text::XTextViewCursor> xCursor = xCursorSupplier->getViewCursor();
        return val::u16string(xCursor->getString().getStr());
    }

    val getParagraphStyle(std::string name, val properties)
    {
        using namespace css;
        using namespace css::uno;
        Reference<container::XNameAccess> xStyles = _paragraphStyles();
        if (!xStyles.is())
            return val::undefined();

        Reference<style::XStyle> xStyle(xStyles->getByName(OUString::fromUtf8(name)), UNO_QUERY);
        Reference<beans::XMultiPropertySet> xStyleProp(xStyle, UNO_QUERY);
        if (!xStyleProp.is())
            return val::undefined();
        const uint32_t len = properties["length"].as<uint32_t>();
        Sequence<rtl::OUString> names = valStrArrayToSequence(properties);
        Sequence<Any> values = xStyleProp->getPropertyValues(names);
        Any* valuesArray = values.getArray();
        val result = val::object();
        result.set("name", val(name));
        for (sal_uInt32 i = 0; i < len; ++i)
        {
            result.set(properties[i], unoAnyToVal(valuesArray[i]));
        }

        return result;
    }

    // Forwarded from IWriterExtensions.hxx, unotxdoc.hxx, wasm/extensions.cxx
    val comments() { return writer()->comments(); }
    void addComment(std::string text) { writer()->addComment(std::move(text)); }
    void replyComment(int parentId, std::string text)
    {
        writer()->replyComment(parentId, std::move(text));
    }
    void deleteCommentThreads(val parentIds) { writer()->deleteCommentThreads(parentIds); }
    void deleteComment(int commentId) { writer()->deleteComment(commentId); }
    void resolveCommentThread(int parentId) { writer()->resolveCommentThread(parentId); }
    void resolveComment(int commentId) { writer()->resolveComment(commentId); }
    void sanitize(val options) { writer()->sanitize(std::move(options)); }
    val pageRects() { return writer()->pageRects(); }
    val headerFooterRect() { return writer()->headerFooterRect(); }
    val paragraphStyles(val properties)
    {
        using namespace css::uno;
        Sequence<rtl::OUString> names = valStrArrayToSequence(properties);
        return writer()->paragraphStyles(unoAnyToVal, _paragraphStyles(), properties, names);
    }

    std::shared_ptr<wasm::ITextRanges> findAll(std::string text, val options)
    {
        return writer()->findAllTextRanges(text, std::move(options));
    }

    val getOutline() { return writer()->getOutline(); }
    val gotoOutline(int idx) { return writer()->gotoOutline(idx); }
    void setAuthor(std::string author) { doc_->setAuthor(author.c_str()); }

private:
    struct DocWithId
    {
        DocumentClient* cli_;
        const int viewId_;
        DocWithId(DocumentClient* doc, int viewId)
            : cli_(doc)
            , viewId_(viewId){};
    };

    const uint32_t ref_;
    lok::Document* doc_ = nullptr;
    std::unordered_map<int, std::unordered_set<int>> subscribed_events_;
    std::unordered_map<int, bool> callback_registered_;
    bool rendering_tiles_ = false;
    css::uno::Reference<css::text::XTextRange> stored_range_;
    std::shared_ptr<wasm::ITextRanges> find_text_ranges_;

    static constexpr int MAX_TILE_DIM = 2048; // this is the de facto max dim for WebGL
    static constexpr int BYTES_PER_PIXEL = 4; /** RGBA **/
    std::array<uint8_t, MAX_TILE_DIM * MAX_TILE_DIM * BYTES_PER_PIXEL> tile_scratch_;

    static void handleCallback(int type, const char* payload, void* docWithId_)
    {
        DocWithId* docWithId = static_cast<DocWithId*>(docWithId_);
        DocumentClient* self = docWithId->cli_;
        // we copy to safePayload because the lifetime of the string is on the stack
        // and the call to the main thread is async
        SafeString safePayload;
        auto subscribed_events_ = self->subscribed_events_[docWithId->viewId_];

        if (subscribed_events_.find(type) != subscribed_events_.end())
        {
            if (!safePayload.first)
                safePayload = makeSafeString(payload);
            MAIN_THREAD_ASYNC_EM_ASM(
                {
                    Module.callbackHandlers.callback($0, $1, UTF8ToString($2));
                    Module.freeSafeString($3);
                },
                self->ref_, type, safePayload.second, safePayload.first);
        }
    }

    void registerCallbackOnce(int viewId)
    {
        // if the viewId is the default, use the default view because registerCallback
        // needs some view to register the callback anyway
        if (viewId < 0)
        {
            viewId = doc_->getView();
            if (viewId < 0)
            {
                SAL_WARN("wasm", "failed to get view id");
                return;
            }
        }

        if (callback_registered_[viewId])
            return;
        doc_->registerCallback(DocumentClient::handleCallback, new DocWithId(this, viewId));
        callback_registered_[viewId] = true;
    }
};

}

EMSCRIPTEN_BINDINGS(lok)
{
    register_optional<bool>();
    register_optional<std::string>();
    register_optional<int>();
    function("preload", &preload);
    function("freeSafeString", &freeSafeString);

    class_<wasm::ITextRanges>("TextRanges")
        .smart_ptr<std::shared_ptr<wasm::ITextRanges>>("TextRanges")
        .function("length", &wasm::ITextRanges::length)
        .function("rect", &wasm::ITextRanges::rect)
        .function("rects", &wasm::ITextRanges::rects)
        .function("isCursorAt", &wasm::ITextRanges::isCursorAt)
        .function("indexAtCursor", &wasm::ITextRanges::indexAtCursor)
        .function("moveCursorTo", &wasm::ITextRanges::moveCursorTo)
        .function("description", &wasm::ITextRanges::description)
        .function("descriptions", &wasm::ITextRanges::descriptions)
        .function("replace", &wasm::ITextRanges::replace)
        .function("replaceAll", &wasm::ITextRanges::replaceAll);

    class_<DocumentClient>("Document")
        .constructor<std::string>()
        .function("valid", &DocumentClient::valid)
        .function("saveAs", &DocumentClient::saveAs)
        .function("getParts", &DocumentClient::getParts)
        .function("pageRects", &DocumentClient::pageRects)
        .function("paintTile", &DocumentClient::paintTile)
        .function("getDocumentSize", &DocumentClient::getDocumentSize)
        .function("initializeForRendering", &DocumentClient::initializeForRendering)
        .function("postTextInputEvent", &DocumentClient::postTextInputEvent)
        .function("postKeyEvent", &DocumentClient::postKeyEvent)
        .function("postMouseEvent", &DocumentClient::postMouseEvent)
        .function("setTextSelection", &DocumentClient::setTextSelection)
        .function("getClipboard", &DocumentClient::getClipboard)
        .function("setClipboard", &DocumentClient::setClipboard)
        .function("paste", &DocumentClient::paste)
        .function("setGraphicSelection", &DocumentClient::setGraphicSelection)
        .function("resetSelection", &DocumentClient::resetSelection)
        .function("getCommandValues", &DocumentClient::getCommandValues)
        .function("subscribe", &DocumentClient::subscribe)
        .function("unsubscribe", &DocumentClient::unsubscribe)
        .function("getViewId", &DocumentClient::getViewId)
        .function("setCurrentView", &DocumentClient::setCurrentView)
        .function("dispatchCommand", &DocumentClient::dispatchCommand)
        .function("removeText", &DocumentClient::removeText)
        .function("startTileRenderer", &DocumentClient::startTileRenderer)
        .function("stopTileRenderer", &DocumentClient::stopTileRenderer)
        .function("ref", &DocumentClient::ref)
        .function("setClientVisibleArea", &DocumentClient::setClientVisibleArea)
        .function("getSelectionText", &DocumentClient::getSelectionText)
        .function("getParagraphStyle", &DocumentClient::getParagraphStyle)
        .function("getPropertyValue", &DocumentClient::getPropertyValue)
        .function("setPropertyValue", &DocumentClient::setPropertyValue)
        .function("saveCurrentSelection", &DocumentClient::saveCurrentSelection)
        .function("restoreCurrentSelection", &DocumentClient::restoreCurrentSelection)
        .function("headerFooterRect", &DocumentClient::headerFooterRect)
        .function("paragraphStyles", &DocumentClient::paragraphStyles)
        .function("findAll", &DocumentClient::findAll)
        .function("comments", &DocumentClient::comments)
        .function("addComment", &DocumentClient::addComment)
        .function("replyComment", &DocumentClient::replyComment)
        .function("deleteCommentThreads", &DocumentClient::deleteCommentThreads)
        .function("deleteComment", &DocumentClient::deleteComment)
        .function("resolveCommentThread", &DocumentClient::resolveCommentThread)
        .function("resolveComment", &DocumentClient::resolveComment)
        .function("sanitize", &DocumentClient::sanitize)
        .function("gotoOutline", &DocumentClient::gotoOutline)
        .function("getOutline", &DocumentClient::getOutline)
        .function("setAuthor", &DocumentClient::setAuthor)
        .function("newView", &DocumentClient::newView);
}
