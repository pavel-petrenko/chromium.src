/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WebFrameInternal.h"

#import "DOMCSSStyleDeclarationInternal.h"
#import "DOMDocumentFragmentInternal.h"
#import "DOMDocumentInternal.h"
#import "DOMElementInternal.h"
#import "DOMHTMLElementInternal.h"
#import "DOMNodeInternal.h"
#import "DOMRangeInternal.h"
#import "WebChromeClient.h"
#import "WebDataSourceInternal.h"
#import "WebDocumentLoaderMac.h"
#import "WebFrameLoaderClient.h"
#import "WebFrameViewInternal.h"
#import "WebHTMLViewInternal.h"
#import "WebKitLogging.h"
#import "WebKitStatisticsPrivate.h"
#import "WebNSURLExtras.h"
#import "WebScriptDebugger.h"
#import "WebViewInternal.h"
#import <JavaScriptCore/APICast.h>
#import <JavaScriptCore/array_object.h>
#import <JavaScriptCore/date_object.h>
#import <WebCore/AXObjectCache.h>
#import <WebCore/ColorMac.h>
#import <WebCore/DOMImplementation.h>
#import <WebCore/DocLoader.h>
#import <WebCore/DocumentFragment.h>
#import <WebCore/Editor.h>
#import <WebCore/EventHandler.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/FrameTree.h>
#import <WebCore/GraphicsContext.h>
#import <WebCore/HTMLFrameOwnerElement.h>
#import <WebCore/HTMLInputElement.h>
#import <WebCore/HistoryItem.h>
#import <WebCore/HitTestResult.h>
#import <WebCore/Page.h>
#import <WebCore/PluginData.h>
#import <WebCore/RenderTreeAsText.h>
#import <WebCore/RenderView.h>
#import <WebCore/RenderWidget.h>
#import <WebCore/ReplaceSelectionCommand.h>
#import <WebCore/SimpleFontData.h>
#import <WebCore/SmartReplace.h>
#import <WebCore/SystemTime.h>
#import <WebCore/TextIterator.h>
#import <WebCore/TextResourceDecoder.h>
#import <WebCore/TypingCommand.h>
#import <WebCore/htmlediting.h>
#import <WebCore/kjs_proxy.h>
#import <WebCore/markup.h>
#import <WebCore/visible_units.h>

using namespace std;
using namespace WebCore;
using namespace HTMLNames;

using KJS::ArrayInstance;
using KJS::BooleanType;
using KJS::DateInstance;
using KJS::ExecState;
using KJS::GetterSetterType;
using KJS::JSGlobalObject;
using KJS::JSLock;
using KJS::JSObject;
using KJS::JSValue;
using KJS::NullType;
using KJS::NumberType;
using KJS::ObjectType;
using KJS::StringType;
using KJS::UndefinedType;
using KJS::UnspecifiedType;

/*
Here is the current behavior matrix for four types of navigations:

Standard Nav:

 Restore form state:   YES
 Restore scroll and focus state:  YES
 Cache policy: NSURLRequestUseProtocolCachePolicy
 Add to back/forward list: YES
 
Back/Forward:

 Restore form state:   YES
 Restore scroll and focus state:  YES
 Cache policy: NSURLRequestReturnCacheDataElseLoad
 Add to back/forward list: NO

Reload (meaning only the reload button):

 Restore form state:   NO
 Restore scroll and focus state:  YES
 Cache policy: NSURLRequestReloadIgnoringCacheData
 Add to back/forward list: NO

Repeat load of the same URL (by any other means of navigation other than the reload button, including hitting return in the location field):

 Restore form state:   NO
 Restore scroll and focus state:  NO, reset to initial conditions
 Cache policy: NSURLRequestReloadIgnoringCacheData
 Add to back/forward list: NO
*/

NSString *WebPageCacheEntryDateKey = @"WebPageCacheEntryDateKey";
NSString *WebPageCacheDataSourceKey = @"WebPageCacheDataSourceKey";
NSString *WebPageCacheDocumentViewKey = @"WebPageCacheDocumentViewKey";

@interface NSView (WebFramePluginHosting)
- (void)setWebFrame:(WebFrame *)webFrame;
@end

@implementation WebFramePrivate

- (void)dealloc
{
    [webFrameView release];

    delete scriptDebugger;

    [super dealloc];
}

- (void)finalize
{
    delete scriptDebugger;

    [super finalize];
}

- (void)setWebFrameView:(WebFrameView *)v 
{ 
    [v retain];
    [webFrameView release];
    webFrameView = v;
}

@end

CSSStyleDeclaration* core(DOMCSSStyleDeclaration *declaration)
{
    return [declaration _CSSStyleDeclaration];
}

DOMCSSStyleDeclaration *kit(WebCore::CSSStyleDeclaration* declaration)
{
    return [DOMCSSStyleDeclaration _wrapCSSStyleDeclaration:declaration];
}

Element* core(DOMElement *element)
{
    return [element _element];
}

DOMElement *kit(Element* element)
{
    return [DOMElement _wrapElement:element];
}

Node* core(DOMNode *node)
{
    return [node _node];
}

DOMNode *kit(Node* node)
{
    return [DOMNode _wrapNode:node];
}

DOMNode *kit(PassRefPtr<Node> node)
{
    return [DOMNode _wrapNode:node.get()];
}

Document* core(DOMDocument *document)
{
    return [document _document];
}

DOMDocument *kit(Document* document)
{
    return [DOMDocument _wrapDocument:document];
}

HTMLElement* core(DOMHTMLElement *element)
{
    return [element _HTMLElement];
}

DOMHTMLElement *kit(HTMLElement *element)
{
    return [DOMHTMLElement _wrapHTMLElement:element];
}

Range* core(DOMRange *range)
{
    return [range _range];
}

DOMRange *kit(Range* range)
{
    return [DOMRange _wrapRange:range];
}

EditableLinkBehavior core(WebKitEditableLinkBehavior editableLinkBehavior)
{
    switch (editableLinkBehavior) {
        case WebKitEditableLinkDefaultBehavior:
            return EditableLinkDefaultBehavior;
        case WebKitEditableLinkAlwaysLive:
            return EditableLinkAlwaysLive;
        case WebKitEditableLinkOnlyLiveWithShiftKey:
            return EditableLinkOnlyLiveWithShiftKey;
        case WebKitEditableLinkLiveWhenNotFocused:
            return EditableLinkLiveWhenNotFocused;
        case WebKitEditableLinkNeverLive:
            return EditableLinkNeverLive;
    }
    ASSERT_NOT_REACHED();
    return EditableLinkDefaultBehavior;
}

@implementation WebFrame (WebInternal)

Frame* core(WebFrame *frame)
{
    return frame ? frame->_private->coreFrame : 0;
}

WebFrame *kit(Frame* frame)
{
    return frame ? static_cast<WebFrameLoaderClient*>(frame->loader()->client())->webFrame() : nil;
}

Page* core(WebView *webView)
{
    return [webView page];
}

WebView *kit(Page* page)
{
    return page ? static_cast<WebChromeClient*>(page->chrome()->client())->webView() : nil;
}

WebView *getWebView(WebFrame *webFrame)
{
    Frame* coreFrame = core(webFrame);
    if (!coreFrame)
        return nil;
    return kit(coreFrame->page());
}

+ (PassRefPtr<Frame>)_createFrameWithPage:(Page*)page frameName:(const String&)name frameView:(WebFrameView *)frameView ownerElement:(HTMLFrameOwnerElement*)ownerElement
{
    WebView *webView = kit(page);

    WebFrame *frame = [[self alloc] _initWithWebFrameView:frameView webView:webView];
    RefPtr<Frame> coreFrame = new Frame(page, ownerElement, new WebFrameLoaderClient(frame));
    [frame release];
    frame->_private->coreFrame = coreFrame.get();

    coreFrame->tree()->setName(name);
    coreFrame->init();

    [webView _setZoomMultiplier:[webView _realZoomMultiplier] isTextOnly:[webView _realZoomMultiplierIsTextOnly]];

    return coreFrame.release();
}

+ (void)_createMainFrameWithPage:(Page*)page frameName:(const String&)name frameView:(WebFrameView *)frameView
{
    [self _createFrameWithPage:page frameName:name frameView:frameView ownerElement:0];
}

+ (PassRefPtr<WebCore::Frame>)_createSubframeWithOwnerElement:(HTMLFrameOwnerElement*)ownerElement frameName:(const String&)name frameView:(WebFrameView *)frameView
{
    return [self _createFrameWithPage:ownerElement->document()->frame()->page() frameName:name frameView:frameView ownerElement:ownerElement];
}

/*
    In the case of saving state about a page with frames, we store a tree of items that mirrors the frame tree.  
    The item that was the target of the user's navigation is designated as the "targetItem".  
    When this method is called with doClip=YES we're able to create the whole tree except for the target's children, 
    which will be loaded in the future.  That part of the tree will be filled out as the child loads are committed.
*/

+ (CFAbsoluteTime)_timeOfLastCompletedLoad
{
    return FrameLoader::timeOfLastCompletedLoad() - kCFAbsoluteTimeIntervalSince1970;
}

- (void)_loadURL:(NSURL *)URL referrer:(NSString *)referrer intoChild:(WebFrame *)childFrame
{
    ASSERT(childFrame);
    HistoryItem* parentItem = core(self)->loader()->currentHistoryItem();
    FrameLoadType loadType = [self _frameLoader]->loadType();
    FrameLoadType childLoadType = FrameLoadTypeRedirectWithLockedHistory;

    // If we're moving in the backforward list, we might want to replace the content
    // of this child frame with whatever was there at that point.
    // Reload will maintain the frame contents, LoadSame will not.
    if (parentItem && parentItem->children().size() &&
        (isBackForwardLoadType(loadType)
         || loadType == FrameLoadTypeReload
         || loadType == FrameLoadTypeReloadAllowingStaleData))
    {
        HistoryItem* childItem = parentItem->childItemWithName([childFrame name]);
        if (childItem) {
            // Use the original URL to ensure we get all the side-effects, such as
            // onLoad handlers, of any redirects that happened. An example of where
            // this is needed is Radar 3213556.
            URL = [NSURL _web_URLWithDataAsString:childItem->originalURLString()];
            // These behaviors implied by these loadTypes should apply to the child frames
            childLoadType = loadType;

            if (isBackForwardLoadType(loadType))
                // For back/forward, remember this item so we can traverse any child items as child frames load
                core(childFrame)->loader()->setProvisionalHistoryItem(childItem);
            else
                // For reload, just reinstall the current item, since a new child frame was created but we won't be creating a new BF item
                core(childFrame)->loader()->setCurrentHistoryItem(childItem);
        }
    }

    WebArchive *archive = [[self _dataSource] _popSubframeArchiveWithName:[childFrame name]];
    if (archive)
        [childFrame loadArchive:archive];
    else
        [childFrame _frameLoader]->load([URL absoluteURL], referrer, childLoadType,
                                        String(), 0, 0);
}


- (void)_viewWillMoveToHostWindow:(NSWindow *)hostWindow
{
    Frame* coreFrame = core(self);
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame))
        [[[kit(frame) frameView] documentView] viewWillMoveToHostWindow:hostWindow];
}

- (void)_viewDidMoveToHostWindow
{
    Frame* coreFrame = core(self);
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame))
        [[[kit(frame) frameView] documentView] viewDidMoveToHostWindow];
}

- (void)_addChild:(WebFrame *)child
{
    core(self)->tree()->appendChild(adoptRef(core(child)));
    if ([child _dataSource])
        [[child _dataSource] _documentLoader]->setOverrideEncoding([[self _dataSource] _documentLoader]->overrideEncoding());  
}

- (int)_numPendingOrLoadingRequests:(BOOL)recurse
{
    return core(self)->loader()->numPendingOrLoadingRequests(recurse);
}

- (void)_attachScriptDebugger
{
    if (_private->scriptDebugger)
        return;

    JSGlobalObject* globalObject = core(self)->scriptProxy()->globalObject();
    if (!globalObject)
        return;

    _private->scriptDebugger = new WebScriptDebugger(globalObject);
}

- (void)_detachScriptDebugger
{
    if (!_private->scriptDebugger)
        return;

    delete _private->scriptDebugger;
    _private->scriptDebugger = 0;
}

- (id)_initWithWebFrameView:(WebFrameView *)fv webView:(WebView *)v
{
    self = [super init];
    if (!self)
        return nil;

    _private = [[WebFramePrivate alloc] init];

    if (fv) {
        [_private setWebFrameView:fv];
        [fv _setWebFrame:self];
    }

    _private->shouldCreateRenderers = YES;

    ++WebFrameCount;

    return self;
}

- (NSArray *)_documentViews
{
    NSMutableArray *result = [NSMutableArray array];
    Frame* coreFrame = core(self);
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        id docView = [[kit(frame) frameView] documentView];
        if (docView)
            [result addObject:docView];
    }
    return result;
}

- (void)_updateBackground
{
    BOOL drawsBackground = [getWebView(self) drawsBackground];
    NSColor *backgroundColor = [getWebView(self) backgroundColor];

    Frame* coreFrame = core(self);
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        WebFrame *webFrame = kit(frame);
        // Never call setDrawsBackground:YES here on the scroll view or the background color will
        // flash between pages loads. setDrawsBackground:YES will be called in _frameLoadCompleted.
        if (!drawsBackground)
            [[[webFrame frameView] _scrollView] setDrawsBackground:NO];
        [[[webFrame frameView] _scrollView] setBackgroundColor:backgroundColor];
        id documentView = [[webFrame frameView] documentView];
        if ([documentView respondsToSelector:@selector(setDrawsBackground:)])
            [documentView setDrawsBackground:drawsBackground];
        if ([documentView respondsToSelector:@selector(setBackgroundColor:)])
            [documentView setBackgroundColor:backgroundColor];
        [self _setDrawsBackground:drawsBackground];
        [self _setBaseBackgroundColor:backgroundColor];
    }
}

- (void)_setInternalLoadDelegate:(id)internalLoadDelegate
{
    _private->internalLoadDelegate = internalLoadDelegate;
}

- (id)_internalLoadDelegate
{
    return _private->internalLoadDelegate;
}

#ifndef BUILDING_ON_TIGER
- (void)_unmarkAllBadGrammar
{
    Frame* coreFrame = core(self);
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        Document *doc = frame->document();
        if (!doc)
            return;

        doc->removeMarkers(DocumentMarker::Grammar);
    }
}
#endif

- (void)_unmarkAllMisspellings
{
    Frame* coreFrame = core(self);
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        Document *doc = frame->document();
        if (!doc)
            return;

        doc->removeMarkers(DocumentMarker::Spelling);
    }
}

- (BOOL)_hasSelection
{
    id documentView = [_private->webFrameView documentView];    

    // optimization for common case to avoid creating potentially large selection string
    if ([documentView isKindOfClass:[WebHTMLView class]])
        if (Frame* coreFrame = core(self))
            return coreFrame->selectionController()->isRange();

    if ([documentView conformsToProtocol:@protocol(WebDocumentText)])
        return [[documentView selectedString] length] > 0;
    
    return NO;
}

- (void)_clearSelection
{
    id documentView = [_private->webFrameView documentView];    
    if ([documentView conformsToProtocol:@protocol(WebDocumentText)])
        [documentView deselectAll];
}

#if !ASSERT_DISABLED
- (BOOL)_atMostOneFrameHasSelection
{
    // FIXME: 4186050 is one known case that makes this debug check fail.
    BOOL found = NO;
    Frame* coreFrame = core(self);
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame))
        if ([kit(frame) _hasSelection]) {
            if (found)
                return NO;
            found = YES;
        }
    return YES;
}
#endif

- (WebFrame *)_findFrameWithSelection
{
    Frame* coreFrame = core(self);
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame))
        if ([kit(frame) _hasSelection])
            return kit(frame);
    return nil;
}

- (void)_clearSelectionInOtherFrames
{
    // We rely on WebDocumentSelection protocol implementors to call this method when they become first 
    // responder. It would be nicer to just notice first responder changes here instead, but there's no 
    // notification sent when the first responder changes in general (Radar 2573089).
    WebFrame *frameWithSelection = [[getWebView(self) mainFrame] _findFrameWithSelection];
    if (frameWithSelection != self)
        [frameWithSelection _clearSelection];

    // While we're in the general area of selection and frames, check that there is only one now.
    ASSERT([[getWebView(self) mainFrame] _atMostOneFrameHasSelection]);
}

- (BOOL)_isMainFrame
{
   Frame* coreFrame = core(self);
   if (!coreFrame)
       return NO;
   return coreFrame == coreFrame->page()->mainFrame() ;
}

- (FrameLoader*)_frameLoader
{
    Frame* frame = core(self);
    return frame ? frame->loader() : 0;
}

static inline WebDataSource *dataSource(DocumentLoader* loader)
{
    return loader ? static_cast<WebDocumentLoaderMac*>(loader)->dataSource() : nil;
}

- (WebDataSource *)_dataSourceForDocumentLoader:(DocumentLoader*)loader
{
    return dataSource(loader);
}

- (void)_addDocumentLoader:(DocumentLoader*)loader toUnarchiveState:(WebArchive *)archive
{
    [dataSource(loader) _addToUnarchiveState:archive];
}

- (WebDataSource *)_dataSource
{
    FrameLoader* frameLoader = [self _frameLoader];

    if (!frameLoader)
        return nil;

    return dataSource(frameLoader->documentLoader());
}

#if ENABLE(NETSCAPE_PLUGIN_API)
- (void)_recursive_resumeNullEventsForAllNetscapePlugins
{
    Frame* coreFrame = core(self);
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        NSView <WebDocumentView> *documentView = [[kit(frame) frameView] documentView];
        if ([documentView isKindOfClass:[WebHTMLView class]])
            [(WebHTMLView *)documentView _resumeNullEventsForAllNetscapePlugins];
    }
}

- (void)_recursive_pauseNullEventsForAllNetscapePlugins
{
    Frame* coreFrame = core(self);
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        NSView <WebDocumentView> *documentView = [[kit(frame) frameView] documentView];
        if ([documentView isKindOfClass:[WebHTMLView class]])
            [(WebHTMLView *)documentView _pauseNullEventsForAllNetscapePlugins];
    }
}
#endif

static NSAppleEventDescriptor* aeDescFromJSValue(ExecState* exec, JSValue* jsValue)
{
    NSAppleEventDescriptor* aeDesc = 0;
    switch (jsValue->type()) {
        case BooleanType:
            aeDesc = [NSAppleEventDescriptor descriptorWithBoolean:jsValue->getBoolean()];
            break;
        case StringType:
            aeDesc = [NSAppleEventDescriptor descriptorWithString:String(jsValue->getString())];
            break;
        case NumberType: {
            double value = jsValue->getNumber();
            int intValue = (int)value;
            if (value == intValue)
                aeDesc = [NSAppleEventDescriptor descriptorWithDescriptorType:typeSInt32 bytes:&intValue length:sizeof(intValue)];
            else
                aeDesc = [NSAppleEventDescriptor descriptorWithDescriptorType:typeIEEE64BitFloatingPoint bytes:&value length:sizeof(value)];
            break;
        }
        case ObjectType: {
            JSObject* object = jsValue->getObject();
            if (object->inherits(&DateInstance::info)) {
                DateInstance* date = static_cast<DateInstance*>(object);
                double ms = 0;
                int tzOffset = 0;
                if (date->getTime(ms, tzOffset)) {
                    CFAbsoluteTime utcSeconds = ms / 1000 - kCFAbsoluteTimeIntervalSince1970;
                    LongDateTime ldt;
                    if (noErr == UCConvertCFAbsoluteTimeToLongDateTime(utcSeconds, &ldt))
                        aeDesc = [NSAppleEventDescriptor descriptorWithDescriptorType:typeLongDateTime bytes:&ldt length:sizeof(ldt)];
                }
            }
            else if (object->inherits(&ArrayInstance::info)) {
                static HashSet<JSObject*> visitedElems;
                if (!visitedElems.contains(object)) {
                    visitedElems.add(object);
                    
                    ArrayInstance* array = static_cast<ArrayInstance*>(object);
                    aeDesc = [NSAppleEventDescriptor listDescriptor];
                    unsigned numItems = array->getLength();
                    for (unsigned i = 0; i < numItems; ++i)
                        [aeDesc insertDescriptor:aeDescFromJSValue(exec, array->getItem(i)) atIndex:0];
                    
                    visitedElems.remove(object);
                }
            }
            if (!aeDesc) {
                JSValue* primitive = object->toPrimitive(exec);
                if (exec->hadException()) {
                    exec->clearException();
                    return [NSAppleEventDescriptor nullDescriptor];
                }
                return aeDescFromJSValue(exec, primitive);
            }
            break;
        }
        case UndefinedType:
            aeDesc = [NSAppleEventDescriptor descriptorWithTypeCode:cMissingValue];
            break;
        default:
            LOG_ERROR("Unknown JavaScript type: %d", jsValue->type());
            // no break;
        case UnspecifiedType:
        case NullType:
        case GetterSetterType:
            aeDesc = [NSAppleEventDescriptor nullDescriptor];
            break;
    }
    
    return aeDesc;
}

- (NSString *)_domain
{
    Document *doc = _private->coreFrame->document();
    if (doc)
        return doc->domain();
    return nil;
}

- (void)_addData:(NSData *)data
{
    Document *doc = _private->coreFrame->document();
    
    // Document may be nil if the part is about to redirect
    // as a result of JS executing during load, i.e. one frame
    // changing another's location before the frame's document
    // has been created. 
    if (doc) {
        doc->setShouldCreateRenderers(_private->shouldCreateRenderers);
        _private->coreFrame->loader()->addData((const char *)[data bytes], [data length]);
    }
}

- (NSString *)_stringWithDocumentTypeStringAndMarkupString:(NSString *)markupString
{
    return _private->coreFrame->documentTypeString() + markupString;
}

- (NSArray *)_nodesFromList:(Vector<Node*> *)nodesVector
{
    size_t size = nodesVector->size();
    NSMutableArray *nodes = [NSMutableArray arrayWithCapacity:size];
    for (size_t i = 0; i < size; ++i)
        [nodes addObject:[DOMNode _wrapNode:(*nodesVector)[i]]];
    return nodes;
}

- (NSString *)_markupStringFromNode:(DOMNode *)node nodes:(NSArray **)nodes
{
    // FIXME: This is never "for interchange". Is that right? See the next method.
    Vector<Node*> nodeList;
    NSString *markupString = createMarkup([node _node], IncludeNode, nodes ? &nodeList : 0);
    if (nodes)
        *nodes = [self _nodesFromList:&nodeList];

    return [self _stringWithDocumentTypeStringAndMarkupString:markupString];
}

- (NSString *)_markupStringFromRange:(DOMRange *)range nodes:(NSArray **)nodes
{
    // FIXME: This is always "for interchange". Is that right? See the previous method.
    Vector<Node*> nodeList;
    NSString *markupString = createMarkup([range _range], nodes ? &nodeList : 0, AnnotateForInterchange);
    if (nodes)
        *nodes = [self _nodesFromList:&nodeList];

    return [self _stringWithDocumentTypeStringAndMarkupString:markupString];
}

- (NSString *)_selectedString
{
    String text = _private->coreFrame->selectedText();
    text.replace('\\', _private->coreFrame->backslashAsCurrencySymbol());
    return text;
}

- (NSString *)_stringForRange:(DOMRange *)range
{
    // This will give a system malloc'd buffer that can be turned directly into an NSString
    unsigned length;
    UChar* buf = plainTextToMallocAllocatedBuffer([range _range], length);
    
    if (!buf)
        return [NSString string];
    
    UChar backslashAsCurrencySymbol = _private->coreFrame->backslashAsCurrencySymbol();
    if (backslashAsCurrencySymbol != '\\')
        for (unsigned n = 0; n < length; n++) 
            if (buf[n] == '\\')
                buf[n] = backslashAsCurrencySymbol;

    // Transfer buffer ownership to NSString
    return [[[NSString alloc] initWithCharactersNoCopy:buf length:length freeWhenDone:YES] autorelease];
}

- (void)_forceLayoutAdjustingViewSize:(BOOL)flag
{
    _private->coreFrame->forceLayout(!flag);
    if (flag)
        _private->coreFrame->view()->adjustViewSize();
}

- (void)_forceLayoutWithMinimumPageWidth:(float)minPageWidth maximumPageWidth:(float)maxPageWidth adjustingViewSize:(BOOL)flag
{
    _private->coreFrame->forceLayoutWithPageWidthRange(minPageWidth, maxPageWidth, flag);
}

- (void)_sendScrollEvent
{
    _private->coreFrame->sendScrollEvent();
}

- (void)_drawRect:(NSRect)rect
{
    PlatformGraphicsContext* platformContext = static_cast<PlatformGraphicsContext*>([[NSGraphicsContext currentContext] graphicsPort]);
    ASSERT([[NSGraphicsContext currentContext] isFlipped]);
    GraphicsContext context(platformContext);
    
    _private->coreFrame->paint(&context, enclosingIntRect(rect));
}

// Used by pagination code called from AppKit when a standalone web page is printed.
- (NSArray*)_computePageRectsWithPrintWidthScaleFactor:(float)printWidthScaleFactor printHeight:(float)printHeight
{
    NSMutableArray* pages = [NSMutableArray arrayWithCapacity:5];
    if (printWidthScaleFactor <= 0) {
        LOG_ERROR("printWidthScaleFactor has bad value %.2f", printWidthScaleFactor);
        return pages;
    }
    
    if (printHeight <= 0) {
        LOG_ERROR("printHeight has bad value %.2f", printHeight);
        return pages;
    }

    if (!_private->coreFrame || !_private->coreFrame->document() || !_private->coreFrame->view()) return pages;
    RenderView* root = static_cast<RenderView *>(_private->coreFrame->document()->renderer());
    if (!root) return pages;
    
    FrameView* view = _private->coreFrame->view();
    if (!view)
        return pages;

    NSView* documentView = view->documentView();
    if (!documentView)
        return pages;

    float currPageHeight = printHeight;
    float docHeight = root->layer()->height();
    float docWidth = root->layer()->width();
    float printWidth = docWidth/printWidthScaleFactor;
    
    // We need to give the part the opportunity to adjust the page height at each step.
    for (float i = 0; i < docHeight; i += currPageHeight) {
        float proposedBottom = min(docHeight, i + printHeight);
        _private->coreFrame->adjustPageHeight(&proposedBottom, i, proposedBottom, i);
        currPageHeight = max(1.0f, proposedBottom - i);
        for (float j = 0; j < docWidth; j += printWidth) {
            NSValue* val = [NSValue valueWithRect: NSMakeRect(j, i, printWidth, currPageHeight)];
            [pages addObject: val];
        }
    }
    
    return pages;
}

// This is to support the case where a webview is embedded in the view that's being printed
- (void)_adjustPageHeightNew:(float *)newBottom top:(float)oldTop bottom:(float)oldBottom limit:(float)bottomLimit
{
    _private->coreFrame->adjustPageHeight(newBottom, oldTop, oldBottom, bottomLimit);
}

- (NSObject *)_copyRenderNode:(RenderObject *)node copier:(id <WebCoreRenderTreeCopier>)copier
{
    NSMutableArray *children = [[NSMutableArray alloc] init];
    for (RenderObject *child = node->firstChild(); child; child = child->nextSibling()) {
        [children addObject:[self _copyRenderNode:child copier:copier]];
    }
          
    NSString *name = [[NSString alloc] initWithUTF8String:node->renderName()];
    
    RenderWidget* renderWidget = node->isWidget() ? static_cast<RenderWidget*>(node) : 0;
    Widget* widget = renderWidget ? renderWidget->widget() : 0;
    NSView *view = widget ? widget->getView() : nil;
    
    int nx, ny;
    node->absolutePosition(nx, ny);
    NSObject *copiedNode = [copier nodeWithName:name
                                       position:NSMakePoint(nx,ny)
                                           rect:NSMakeRect(node->xPos(), node->yPos(), node->width(), node->height())
                                           view:view
                                       children:children];
    
    [name release];
    [children release];
    
    return copiedNode;
}

- (NSObject *)_copyRenderTree:(id <WebCoreRenderTreeCopier>)copier
{
    RenderObject *renderer = _private->coreFrame->renderer();
    if (!renderer) {
        return nil;
    }
    return [self _copyRenderNode:renderer copier:copier];
}

static HTMLInputElement* inputElementFromDOMElement(DOMElement* element)
{
    Node* node = [element _node];
    if (node->hasTagName(inputTag))
        return static_cast<HTMLInputElement*>(node);
    return nil;
}

static HTMLFormElement *formElementFromDOMElement(DOMElement *element)
{
    Node *node = [element _node];
    // This should not be necessary, but an XSL file on
    // maps.google.com crashes otherwise because it is an xslt file
    // that contains <form> elements that aren't in any namespace, so
    // they come out as generic CML elements
    if (node && node->hasTagName(formTag)) {
        return static_cast<HTMLFormElement *>(node);
    }
    return nil;
}

- (DOMElement *)_elementWithName:(NSString *)name inForm:(DOMElement *)form
{
    HTMLFormElement *formElement = formElementFromDOMElement(form);
    if (formElement) {
        Vector<HTMLGenericFormElement*>& elements = formElement->formElements;
        AtomicString targetName = name;
        for (unsigned int i = 0; i < elements.size(); i++) {
            HTMLGenericFormElement *elt = elements[i];
            // Skip option elements, other duds
            if (elt->name() == targetName)
                return [DOMElement _wrapElement:elt];
        }
    }
    return nil;
}

- (BOOL)_elementDoesAutoComplete:(DOMElement *)element
{
    HTMLInputElement *inputElement = inputElementFromDOMElement(element);
    return inputElement != nil
        && inputElement->inputType() == HTMLInputElement::TEXT
        && inputElement->autoComplete();
}

- (BOOL)_elementIsPassword:(DOMElement *)element
{
    HTMLInputElement *inputElement = inputElementFromDOMElement(element);
    return inputElement != nil
        && inputElement->inputType() == HTMLInputElement::PASSWORD;
}

- (DOMElement *)_formForElement:(DOMElement *)element;
{
    HTMLInputElement *inputElement = inputElementFromDOMElement(element);
    if (inputElement) {
        HTMLFormElement *formElement = inputElement->form();
        if (formElement) {
            return [DOMElement _wrapElement:formElement];
        }
    }
    return nil;
}

- (DOMElement *)_currentForm
{
    return [DOMElement _wrapElement:_private->coreFrame->currentForm()];
}

- (NSArray *)_controlsInForm:(DOMElement *)form
{
    NSMutableArray *results = nil;
    HTMLFormElement *formElement = formElementFromDOMElement(form);
    if (formElement) {
        Vector<HTMLGenericFormElement*>& elements = formElement->formElements;
        for (unsigned int i = 0; i < elements.size(); i++) {
            if (elements.at(i)->isEnumeratable()) { // Skip option elements, other duds
                DOMElement *de = [DOMElement _wrapElement:elements.at(i)];
                if (!results) {
                    results = [NSMutableArray arrayWithObject:de];
                } else {
                    [results addObject:de];
                }
            }
        }
    }
    return results;
}

- (NSString *)_searchForLabels:(NSArray *)labels beforeElement:(DOMElement *)element
{
    return _private->coreFrame->searchForLabelsBeforeElement(labels, [element _element]);
}

- (NSString *)_matchLabels:(NSArray *)labels againstElement:(DOMElement *)element
{
    return _private->coreFrame->matchLabelsAgainstElement(labels, [element _element]);
}

- (NSURL *)_URLWithAttributeString:(NSString *)string
{
    Document* doc = _private->coreFrame->document();
    if (!doc)
        return nil;
    // FIXME: is parseURL appropriate here?
    return doc->completeURL(parseURL(string));
}

- (BOOL)_searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag startInSelection:(BOOL)startInSelection
{
    return _private->coreFrame->findString(string, forward, caseFlag, wrapFlag, startInSelection);
}

- (unsigned)_markAllMatchesForText:(NSString *)string caseSensitive:(BOOL)caseFlag limit:(unsigned)limit
{
    return _private->coreFrame->markAllMatchesForText(string, caseFlag, limit);
}

- (BOOL)_markedTextMatchesAreHighlighted
{
    return _private->coreFrame->markedTextMatchesAreHighlighted();
}

- (void)_setMarkedTextMatchesAreHighlighted:(BOOL)doHighlight
{
    _private->coreFrame->setMarkedTextMatchesAreHighlighted(doHighlight);
}

- (void)_unmarkAllTextMatches
{
    Document *doc = _private->coreFrame->document();
    if (!doc) {
        return;
    }
    doc->removeMarkers(DocumentMarker::TextMatch);
}

- (NSArray *)_rectsForTextMatches
{
    Document *doc = _private->coreFrame->document();
    if (!doc)
        return [NSArray array];
    
    NSMutableArray *result = [NSMutableArray array];
    Vector<IntRect> rects = doc->renderedRectsForMarkers(DocumentMarker::TextMatch);
    unsigned count = rects.size();
    for (unsigned index = 0; index < count; ++index)
        [result addObject:[NSValue valueWithRect:rects[index]]];
    
    return result;
}

- (NSString *)_stringByEvaluatingJavaScriptFromString:(NSString *)string
{
    return [self _stringByEvaluatingJavaScriptFromString:string forceUserGesture:true];
}

- (NSString *)_stringByEvaluatingJavaScriptFromString:(NSString *)string forceUserGesture:(BOOL)forceUserGesture
{
    ASSERT(_private->coreFrame->document());
    
    JSValue* result = _private->coreFrame->loader()->executeScript(string, forceUserGesture);

    if (!_private->coreFrame) // In case the script removed our frame from the page.
        return @"";

    // This bizarre set of rules matches behavior from WebKit for Safari 2.0.
    // If you don't like it, use -[WebScriptObject evaluateWebScript:] or 
    // JSEvaluateScript instead, since they have less surprising semantics.
    if (!result || !result->isBoolean() && !result->isString() && !result->isNumber())
        return @"";

    JSLock lock;
    return String(result->toString(_private->coreFrame->scriptProxy()->globalObject()->globalExec()));
}

- (NSAppleEventDescriptor *)_aeDescByEvaluatingJavaScriptFromString:(NSString *)string
{
    ASSERT(_private->coreFrame->document());
    ASSERT(_private->coreFrame == _private->coreFrame->page()->mainFrame());
    JSValue* result = _private->coreFrame->loader()->executeScript(string, true);
    if (!result) // FIXME: pass errors
        return 0;
    JSLock lock;
    return aeDescFromJSValue(_private->coreFrame->scriptProxy()->globalObject()->globalExec(), result);
}

- (NSRect)_caretRectAtNode:(DOMNode *)node offset:(int)offset affinity:(NSSelectionAffinity)affinity
{
    return [node _node]->renderer()->caretRect(offset, static_cast<EAffinity>(affinity));
}

- (NSRect)_firstRectForDOMRange:(DOMRange *)range
{
   return _private->coreFrame->firstRectForRange([range _range]);
}

- (void)_scrollDOMRangeToVisible:(DOMRange *)range
{
    NSRect rangeRect = [self _firstRectForDOMRange:range];    
    Node *startNode = [[range startContainer] _node];
        
    if (startNode && startNode->renderer()) {
        RenderLayer *layer = startNode->renderer()->enclosingLayer();
        if (layer)
            layer->scrollRectToVisible(enclosingIntRect(rangeRect), RenderLayer::gAlignToEdgeIfNeeded, RenderLayer::gAlignToEdgeIfNeeded);
    }
}

- (NSURL *)_baseURL
{
    return _private->coreFrame->document()->baseURL();
}

- (NSString *)_stringWithData:(NSData *)data
{
    Document* doc = _private->coreFrame->document();
    if (!doc)
        return nil;
    TextResourceDecoder* decoder = doc->decoder();
    if (!decoder)
        return nil;
    return decoder->encoding().decode(reinterpret_cast<const char*>([data bytes]), [data length]);
}

+ (NSString *)_stringWithData:(NSData *)data textEncodingName:(NSString *)textEncodingName
{
    WebCore::TextEncoding encoding(textEncodingName);
    if (!encoding.isValid())
        encoding = WindowsLatin1Encoding();
    return encoding.decode(reinterpret_cast<const char*>([data bytes]), [data length]);
}

- (BOOL)_needsLayout
{
    return _private->coreFrame->view() ? _private->coreFrame->view()->needsLayout() : false;
}

- (NSString *)_renderTreeAsExternalRepresentation
{
    return externalRepresentation(_private->coreFrame->renderer());
}

- (id)_accessibilityTree
{
    AXObjectCache::enableAccessibility();
    if (!_private->coreFrame || !_private->coreFrame->document())
        return nil;
    RenderView* root = static_cast<RenderView *>(_private->coreFrame->document()->renderer());
    if (!root)
        return nil;
    return _private->coreFrame->document()->axObjectCache()->get(root);
}

- (void)_setBaseBackgroundColor:(NSColor *)backgroundColor
{
    if (_private->coreFrame && _private->coreFrame->view()) {
        Color color = colorFromNSColor([backgroundColor colorUsingColorSpaceName:NSDeviceRGBColorSpace]);
        _private->coreFrame->view()->setBaseBackgroundColor(color);
    }
}

- (void)_setDrawsBackground:(BOOL)drawsBackground
{
    if (_private->coreFrame && _private->coreFrame->view())
        _private->coreFrame->view()->setTransparent(!drawsBackground);
}

- (DOMRange *)_rangeByAlteringCurrentSelection:(SelectionController::EAlteration)alteration direction:(SelectionController::EDirection)direction granularity:(TextGranularity)granularity
{
    if (_private->coreFrame->selectionController()->isNone())
        return nil;

    SelectionController selectionController;
    selectionController.setSelection(_private->coreFrame->selectionController()->selection());
    selectionController.modify(alteration, direction, granularity);
    return [DOMRange _wrapRange:selectionController.toRange().get()];
}

- (TextGranularity)_selectionGranularity
{
    return _private->coreFrame->selectionGranularity();
}

- (NSRange)_convertToNSRange:(Range *)range
{
    if (!range || !range->startContainer())
        return NSMakeRange(NSNotFound, 0);

    Element* selectionRoot = _private->coreFrame->selectionController()->rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : _private->coreFrame->document()->documentElement();
    
    // Mouse events may cause TSM to attempt to create an NSRange for a portion of the view
    // that is not inside the current editable region.  These checks ensure we don't produce
    // potentially invalid data when responding to such requests.
    if (range->startContainer() != scope && !range->startContainer()->isDescendantOf(scope))
        return NSMakeRange(NSNotFound, 0);
    if (range->endContainer() != scope && !range->endContainer()->isDescendantOf(scope))
        return NSMakeRange(NSNotFound, 0);
    
    RefPtr<Range> testRange = Range::create(scope->document(), scope, 0, range->startContainer(), range->startOffset());
    ASSERT(testRange->startContainer() == scope);
    int startPosition = TextIterator::rangeLength(testRange.get());

    ExceptionCode ec;
    testRange->setEnd(range->endContainer(), range->endOffset(), ec);
    ASSERT(testRange->startContainer() == scope);
    int endPosition = TextIterator::rangeLength(testRange.get());

    return NSMakeRange(startPosition, endPosition - startPosition);
}

- (PassRefPtr<Range>)_convertToDOMRange:(NSRange)nsrange
{
    if (nsrange.location > INT_MAX)
        return 0;
    if (nsrange.length > INT_MAX || nsrange.location + nsrange.length > INT_MAX)
        nsrange.length = INT_MAX - nsrange.location;

    // our critical assumption is that we are only called by input methods that
    // concentrate on a given area containing the selection
    // We have to do this because of text fields and textareas. The DOM for those is not
    // directly in the document DOM, so serialization is problematic. Our solution is
    // to use the root editable element of the selection start as the positional base.
    // That fits with AppKit's idea of an input context.
    Element* selectionRoot = _private->coreFrame->selectionController()->rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : _private->coreFrame->document()->documentElement();
    return TextIterator::rangeFromLocationAndLength(scope, nsrange.location, nsrange.length);
}

- (DOMRange *)_convertNSRangeToDOMRange:(NSRange)nsrange
{
    return [DOMRange _wrapRange:[self _convertToDOMRange:nsrange].get()];
}

- (NSRange)_convertDOMRangeToNSRange:(DOMRange *)range
{
    return [self _convertToNSRange:[range _range]];
}

- (DOMRange *)_markDOMRange
{
    return [DOMRange _wrapRange:_private->coreFrame->mark().toRange().get()];
}

- (NSRange)_markedTextNSRange
{
    return [self _convertToNSRange:_private->coreFrame->editor()->compositionRange().get()];
}

// Given proposedRange, returns an extended range that includes adjacent whitespace that should
// be deleted along with the proposed range in order to preserve proper spacing and punctuation of
// the text surrounding the deletion.
- (DOMRange *)_smartDeleteRangeForProposedRange:(DOMRange *)proposedRange
{
    Node *startContainer = [[proposedRange startContainer] _node];
    Node *endContainer = [[proposedRange endContainer] _node];
    if (startContainer == nil || endContainer == nil)
        return nil;

    ASSERT(startContainer->document() == endContainer->document());
    
    _private->coreFrame->document()->updateLayoutIgnorePendingStylesheets();

    Position start(startContainer, [proposedRange startOffset]);
    Position end(endContainer, [proposedRange endOffset]);
    Position newStart = start.upstream().leadingWhitespacePosition(DOWNSTREAM, true);
    if (newStart.isNull())
        newStart = start;
    Position newEnd = end.downstream().trailingWhitespacePosition(DOWNSTREAM, true);
    if (newEnd.isNull())
        newEnd = end;

    newStart = rangeCompliantEquivalent(newStart);
    newEnd = rangeCompliantEquivalent(newEnd);

    RefPtr<Range> range = _private->coreFrame->document()->createRange();
    int exception = 0;
    range->setStart(newStart.node(), newStart.offset(), exception);
    range->setEnd(newStart.node(), newStart.offset(), exception);
    return [DOMRange _wrapRange:range.get()];
}

// Determines whether whitespace needs to be added around aString to preserve proper spacing and
// punctuation when it‚Äôs inserted into the receiver‚Äôs text over charRange. Returns by reference
// in beforeString and afterString any whitespace that should be added, unless either or both are
// nil. Both are returned as nil if aString is nil or if smart insertion and deletion are disabled.
- (void)_smartInsertForString:(NSString *)pasteString replacingRange:(DOMRange *)rangeToReplace beforeString:(NSString **)beforeString afterString:(NSString **)afterString
{
    // give back nil pointers in case of early returns
    if (beforeString)
        *beforeString = nil;
    if (afterString)
        *afterString = nil;
        
    // inspect destination
    Node *startContainer = [[rangeToReplace startContainer] _node];
    Node *endContainer = [[rangeToReplace endContainer] _node];

    Position startPos(startContainer, [rangeToReplace startOffset]);
    Position endPos(endContainer, [rangeToReplace endOffset]);

    VisiblePosition startVisiblePos = VisiblePosition(startPos, VP_DEFAULT_AFFINITY);
    VisiblePosition endVisiblePos = VisiblePosition(endPos, VP_DEFAULT_AFFINITY);
    
    // this check also ensures startContainer, startPos, endContainer, and endPos are non-null
    if (startVisiblePos.isNull() || endVisiblePos.isNull())
        return;

    bool addLeadingSpace = startPos.leadingWhitespacePosition(VP_DEFAULT_AFFINITY, true).isNull() && !isStartOfParagraph(startVisiblePos);
    if (addLeadingSpace)
        if (UChar previousChar = startVisiblePos.previous().characterAfter())
            addLeadingSpace = !isCharacterSmartReplaceExempt(previousChar, true);
    
    bool addTrailingSpace = endPos.trailingWhitespacePosition(VP_DEFAULT_AFFINITY, true).isNull() && !isEndOfParagraph(endVisiblePos);
    if (addTrailingSpace)
        if (UChar thisChar = endVisiblePos.characterAfter())
            addTrailingSpace = !isCharacterSmartReplaceExempt(thisChar, false);
    
    // inspect source
    bool hasWhitespaceAtStart = false;
    bool hasWhitespaceAtEnd = false;
    unsigned pasteLength = [pasteString length];
    if (pasteLength > 0) {
        NSCharacterSet *whiteSet = [NSCharacterSet whitespaceAndNewlineCharacterSet];
        
        if ([whiteSet characterIsMember:[pasteString characterAtIndex:0]]) {
            hasWhitespaceAtStart = YES;
        }
        if ([whiteSet characterIsMember:[pasteString characterAtIndex:(pasteLength - 1)]]) {
            hasWhitespaceAtEnd = YES;
        }
    }
    
    // issue the verdict
    if (beforeString && addLeadingSpace && !hasWhitespaceAtStart)
        *beforeString = @" ";
    if (afterString && addTrailingSpace && !hasWhitespaceAtEnd)
        *afterString = @" ";
}

- (DOMDocumentFragment *)_documentFragmentWithMarkupString:(NSString *)markupString baseURLString:(NSString *)baseURLString 
{
    if (!_private->coreFrame || !_private->coreFrame->document())
        return 0;

    return [DOMDocumentFragment _wrapDocumentFragment:createFragmentFromMarkup(_private->coreFrame->document(), markupString, baseURLString).get()];
}

- (DOMDocumentFragment *)_documentFragmentWithText:(NSString *)text inContext:(DOMRange *)context
{
    return [DOMDocumentFragment _wrapDocumentFragment:createFragmentFromText([context _range], text).get()];
}

- (DOMDocumentFragment *)_documentFragmentWithNodesAsParagraphs:(NSArray *)nodes
{
    if (!_private->coreFrame || !_private->coreFrame->document())
        return 0;
    
    NSEnumerator *nodeEnum = [nodes objectEnumerator];
    Vector<Node*> nodesVector;
    DOMNode *node;
    while ((node = [nodeEnum nextObject]))
        nodesVector.append([node _node]);
    
    return [DOMDocumentFragment _wrapDocumentFragment:createFragmentFromNodes(_private->coreFrame->document(), nodesVector).get()];
}

- (void)_replaceSelectionWithFragment:(DOMDocumentFragment *)fragment selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace matchStyle:(BOOL)matchStyle
{
    if (_private->coreFrame->selectionController()->isNone() || !fragment)
        return;
    
    applyCommand(new ReplaceSelectionCommand(_private->coreFrame->document(), [fragment _documentFragment], selectReplacement, smartReplace, matchStyle));
    _private->coreFrame->revealSelection(RenderLayer::gAlignToEdgeIfNeeded);
}

- (void)_replaceSelectionWithNode:(DOMNode *)node selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace matchStyle:(BOOL)matchStyle
{
    DOMDocumentFragment *fragment = [DOMDocumentFragment _wrapDocumentFragment:_private->coreFrame->document()->createDocumentFragment().get()];
    [fragment appendChild:node];
    [self _replaceSelectionWithFragment:fragment selectReplacement:selectReplacement smartReplace:smartReplace matchStyle:matchStyle];
}

- (void)_replaceSelectionWithMarkupString:(NSString *)markupString baseURLString:(NSString *)baseURLString selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace
{
    DOMDocumentFragment *fragment = [self _documentFragmentWithMarkupString:markupString baseURLString:baseURLString];
    [self _replaceSelectionWithFragment:fragment selectReplacement:selectReplacement smartReplace:smartReplace matchStyle:NO];
}

- (void)_replaceSelectionWithText:(NSString *)text selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace
{
    [self _replaceSelectionWithFragment:[self _documentFragmentWithText:text
        inContext:[DOMRange _wrapRange:_private->coreFrame->selectionController()->toRange().get()]]
        selectReplacement:selectReplacement smartReplace:smartReplace matchStyle:YES];
}

- (void)_insertParagraphSeparatorInQuotedContent
{
    if (_private->coreFrame->selectionController()->isNone())
        return;
    
    TypingCommand::insertParagraphSeparatorInQuotedContent(_private->coreFrame->document());
    _private->coreFrame->revealSelection(RenderLayer::gAlignToEdgeIfNeeded);
}

- (VisiblePosition)_visiblePositionForPoint:(NSPoint)point
{
    IntPoint outerPoint(point);
    HitTestResult result = _private->coreFrame->eventHandler()->hitTestResultAtPoint(outerPoint, true);
    Node* node = result.innerNode();
    if (!node)
        return VisiblePosition();
    RenderObject* renderer = node->renderer();
    if (!renderer)
        return VisiblePosition();
    VisiblePosition visiblePos = renderer->positionForCoordinates(result.localPoint().x(), result.localPoint().y());
    if (visiblePos.isNull())
        visiblePos = VisiblePosition(Position(node, 0));
    return visiblePos;
}

- (DOMRange *)_characterRangeAtPoint:(NSPoint)point
{
    VisiblePosition position = [self _visiblePositionForPoint:point];
    if (position.isNull())
        return nil;
    
    VisiblePosition previous = position.previous();
    if (previous.isNotNull()) {
        DOMRange *previousCharacterRange = [DOMRange _wrapRange:makeRange(previous, position).get()];
        NSRect rect = [self _firstRectForDOMRange:previousCharacterRange];
        if (NSPointInRect(point, rect))
            return previousCharacterRange;
    }

    VisiblePosition next = position.next();
    if (next.isNotNull()) {
        DOMRange *nextCharacterRange = [DOMRange _wrapRange:makeRange(position, next).get()];
        NSRect rect = [self _firstRectForDOMRange:nextCharacterRange];
        if (NSPointInRect(point, rect))
            return nextCharacterRange;
    }
    
    return nil;
}

- (DOMCSSStyleDeclaration *)_typingStyle
{
    if (!_private->coreFrame || !_private->coreFrame->typingStyle())
        return nil;
    return [DOMCSSStyleDeclaration _wrapCSSStyleDeclaration:_private->coreFrame->typingStyle()->copy().get()];
}

- (void)_setTypingStyle:(DOMCSSStyleDeclaration *)style withUndoAction:(EditAction)undoAction
{
    if (!_private->coreFrame)
        return;
    _private->coreFrame->computeAndSetTypingStyle([style _CSSStyleDeclaration], undoAction);
}

- (NSFont *)_fontForSelection:(BOOL *)hasMultipleFonts
{
    bool multipleFonts = false;
    NSFont *font = nil;
    if (_private->coreFrame) {
        const SimpleFontData* fd = _private->coreFrame->editor()->fontForSelection(multipleFonts);
        if (fd)
            font = fd->getNSFont();
    }
    
    if (hasMultipleFonts)
        *hasMultipleFonts = multipleFonts;
    return font;
}

- (void)_dragSourceMovedTo:(NSPoint)windowLoc
{
    if (!_private->coreFrame)
        return;
    FrameView* view = _private->coreFrame->view();
    if (!view)
        return;
    // FIXME: These are fake modifier keys here, but they should be real ones instead.
    PlatformMouseEvent event(IntPoint(windowLoc), globalPoint(windowLoc, [view->getView() window]),
        LeftButton, MouseEventMoved, 0, false, false, false, false, currentTime());
    _private->coreFrame->eventHandler()->dragSourceMovedTo(event);
}

- (void)_dragSourceEndedAt:(NSPoint)windowLoc operation:(NSDragOperation)operation
{
    if (!_private->coreFrame)
        return;
    FrameView* view = _private->coreFrame->view();
    if (!view)
        return;
    // FIXME: These are fake modifier keys here, but they should be real ones instead.
    PlatformMouseEvent event(IntPoint(windowLoc), globalPoint(windowLoc, [view->getView() window]),
        LeftButton, MouseEventMoved, 0, false, false, false, false, currentTime());
    _private->coreFrame->eventHandler()->dragSourceEndedAt(event, (DragOperation)operation);
}

- (BOOL)_getData:(NSData **)data andResponse:(NSURLResponse **)response forURL:(NSString *)url
{
    Document* doc = _private->coreFrame->document();
    if (!doc)
        return NO;

    CachedResource* resource = doc->docLoader()->cachedResource(url);
    if (!resource)
        return NO;

    SharedBuffer* buffer = resource->data();
    if (buffer)
        *data = [buffer->createNSData() autorelease];
    else
        *data = nil;

    *response = resource->response().nsURLResponse();
    return YES;
}

- (void)_getAllResourceDatas:(NSArray **)datas andResponses:(NSArray **)responses
{
    Document* doc = _private->coreFrame->document();
    if (!doc) {
        NSArray* emptyArray = [NSArray array];
        *datas = emptyArray;
        *responses = emptyArray;
        return;
    }

    const HashMap<String, CachedResource*>& allResources = doc->docLoader()->allCachedResources();

    NSMutableArray *d = [[NSMutableArray alloc] initWithCapacity:allResources.size()];
    NSMutableArray *r = [[NSMutableArray alloc] initWithCapacity:allResources.size()];

    HashMap<String, CachedResource*>::const_iterator end = allResources.end();
    for (HashMap<String, CachedResource*>::const_iterator it = allResources.begin(); it != end; ++it) {
        SharedBuffer* buffer = it->second->data();
        NSData *data;
        if (buffer)
            data = buffer->createNSData();
        else
            data = [[NSData alloc] init];
        [d addObject:data];
        [data release];
        [r addObject:it->second->response().nsURLResponse()];
    }

    *datas = [d autorelease];
    *responses = [r autorelease];
}

- (BOOL)_canProvideDocumentSource
{
    String mimeType = _private->coreFrame->loader()->responseMIMEType();
    
    if (WebCore::DOMImplementation::isTextMIMEType(mimeType) ||
        Image::supportsType(mimeType) ||
        (_private->coreFrame->page() && _private->coreFrame->page()->pluginData()->supportsMimeType(mimeType)))
        return NO;
    
    return YES;
}

- (BOOL)_canSaveAsWebArchive
{
    // Currently, all documents that we can view source for
    // (HTML and XML documents) can also be saved as web archives
    return [self _canProvideDocumentSource];
}

- (void)_receivedData:(NSData *)data textEncodingName:(NSString *)textEncodingName
{
    // Set the encoding. This only needs to be done once, but it's harmless to do it again later.
    String encoding;
    if (_private->coreFrame)
        encoding = _private->coreFrame->loader()->documentLoader()->overrideEncoding();
    bool userChosen = !encoding.isNull();
    if (encoding.isNull())
        encoding = textEncodingName;
    _private->coreFrame->loader()->setEncoding(encoding, userChosen);
    [self _addData:data];
}

@end

@implementation WebFrame (WebPrivate)

// FIXME: Yhis exists only as a convenience for Safari, consider moving there.
- (BOOL)_isDescendantOfFrame:(WebFrame *)ancestor
{
    Frame* coreFrame = core(self);
    return coreFrame && coreFrame->tree()->isDescendantOf(core(ancestor));
}

- (void)_setShouldCreateRenderers:(BOOL)shouldCreateRenderers
{
    _private->shouldCreateRenderers = shouldCreateRenderers;
}

- (NSColor *)_bodyBackgroundColor
{
    Document* document = core(self)->document();
    if (!document)
        return nil;
    HTMLElement* body = document->body();
    if (!body)
        return nil;
    RenderObject* bodyRenderer = body->renderer();
    if (!bodyRenderer)
        return nil;
    Color color = bodyRenderer->style()->backgroundColor();
    if (!color.isValid())
        return nil;
    return nsColor(color);
}

- (BOOL)_isFrameSet
{
    return core(self)->isFrameSet();
}

- (BOOL)_firstLayoutDone
{
    return [self _frameLoader]->firstLayoutDone();
}

- (WebFrameLoadType)_loadType
{
    return (WebFrameLoadType)[self _frameLoader]->loadType();
}

- (NSRange)_selectedNSRange
{
    return [self _convertToNSRange:_private->coreFrame->selectionController()->toRange().get()];
}

- (void)_selectNSRange:(NSRange)range
{
    RefPtr<Range> domRange = [self _convertToDOMRange:range];
    if (domRange)
        _private->coreFrame->selectionController()->setSelection(Selection(domRange.get(), SEL_DEFAULT_AFFINITY));
}

- (BOOL)_isDisplayingStandaloneImage
{
    Document* document = core(self)->document();
    return document && document->isImageDocument();
}

@end

@implementation WebFrame

- (id)init
{
    return nil;
}

// Should be deprecated.
- (id)initWithName:(NSString *)name webFrameView:(WebFrameView *)view webView:(WebView *)webView
{
    return nil;
}

- (void)dealloc
{
    [_private release];
    --WebFrameCount;
    [super dealloc];
}

- (void)finalize
{
    --WebFrameCount;
    [super finalize];
}

- (NSString *)name
{
    Frame* coreFrame = core(self);
    if (!coreFrame)
        return nil;
    return coreFrame->tree()->name();
}

- (WebFrameView *)frameView
{
    return _private->webFrameView;
}

- (WebView *)webView
{
    return getWebView(self);
}

- (DOMDocument *)DOMDocument
{
    Frame* coreFrame = core(self);
    if (!coreFrame)
        return nil;
    
    // FIXME: <rdar://problem/5145841> When loading a custom view/representation 
    // into a web frame, the old document can still be around. This makes sure that
    // we'll return nil in those cases.
    if (![[self _dataSource] _isDocumentHTML]) 
        return nil; 

    Document* document = coreFrame->document();
    
    // According to the documentation, we should return nil if the frame doesn't have a document.
    // While full-frame images and plugins do have an underlying HTML document, we return nil here to be
    // backwards compatible.
    if (document && (document->isPluginDocument() || document->isImageDocument()))
        return nil;
    
    return kit(coreFrame->document());
}

- (DOMHTMLElement *)frameElement
{
    Frame* coreFrame = core(self);
    if (!coreFrame)
        return nil;
    return kit(coreFrame->ownerElement());
}

- (WebDataSource *)provisionalDataSource
{
    FrameLoader* frameLoader = [self _frameLoader];
    return frameLoader ? dataSource(frameLoader->provisionalDocumentLoader()) : nil;
}

- (WebDataSource *)dataSource
{
    FrameLoader* loader = [self _frameLoader];
    if (!loader || !loader->frameHasLoaded())
        return nil;

    return [self _dataSource];
}

- (void)loadRequest:(NSURLRequest *)request
{
    [self _frameLoader]->load(request);
}

static NSURL *createUniqueWebDataURL()
{
    CFUUIDRef UUIDRef = CFUUIDCreate(kCFAllocatorDefault);
    NSString *UUIDString = (NSString *)CFUUIDCreateString(kCFAllocatorDefault, UUIDRef);
    CFRelease(UUIDRef);
    NSURL *URL = [NSURL URLWithString:[NSString stringWithFormat:@"applewebdata://%@", UUIDString]];
    CFRelease(UUIDString);
    return URL;
}

- (void)_loadData:(NSData *)data MIMEType:(NSString *)MIMEType textEncodingName:(NSString *)encodingName baseURL:(NSURL *)baseURL unreachableURL:(NSURL *)unreachableURL
{
    KURL responseURL;
    if (!baseURL) {
        baseURL = blankURL();
        responseURL = createUniqueWebDataURL();
    }
    
    ResourceRequest request([baseURL absoluteURL]);

    // hack because Mail checks for this property to detect data / archive loads
    [NSURLProtocol setProperty:@"" forKey:@"WebDataRequest" inRequest:(NSMutableURLRequest *)request.nsURLRequest()];

    SubstituteData substituteData(WebCore::SharedBuffer::wrapNSData(data), MIMEType, encodingName, [unreachableURL absoluteURL], responseURL);

    [self _frameLoader]->load(request, substituteData);
}


- (void)loadData:(NSData *)data MIMEType:(NSString *)MIMEType textEncodingName:(NSString *)encodingName baseURL:(NSURL *)baseURL
{
    if (!MIMEType)
        MIMEType = @"text/html";
    [self _loadData:data MIMEType:MIMEType textEncodingName:encodingName baseURL:baseURL unreachableURL:nil];
}

- (void)_loadHTMLString:(NSString *)string baseURL:(NSURL *)baseURL unreachableURL:(NSURL *)unreachableURL
{
    NSData *data = [string dataUsingEncoding:NSUTF8StringEncoding];
    [self _loadData:data MIMEType:@"text/html" textEncodingName:@"UTF-8" baseURL:baseURL unreachableURL:unreachableURL];
}

- (void)loadHTMLString:(NSString *)string baseURL:(NSURL *)baseURL
{
    [self _loadHTMLString:string baseURL:baseURL unreachableURL:nil];
}

- (void)loadAlternateHTMLString:(NSString *)string baseURL:(NSURL *)baseURL forUnreachableURL:(NSURL *)unreachableURL
{
    [self _loadHTMLString:string baseURL:baseURL unreachableURL:unreachableURL];
}

- (void)loadArchive:(WebArchive *)archive
{
    WebResource *mainResource = [archive mainResource];
    if (mainResource) {
        SubstituteData substituteData(WebCore::SharedBuffer::wrapNSData([mainResource data]), [mainResource MIMEType], [mainResource textEncodingName], KURL());
        ResourceRequest request([mainResource URL]);

        // hack because Mail checks for this property to detect data / archive loads
        [NSURLProtocol setProperty:@"" forKey:@"WebDataRequest" inRequest:(NSMutableURLRequest *)request.nsURLRequest()];

        RefPtr<DocumentLoader> documentLoader = core(self)->loader()->client()->createDocumentLoader(request, substituteData);

        [dataSource(documentLoader.get()) _addToUnarchiveState:archive];

        [self _frameLoader]->load(documentLoader.get());
    }
}

- (void)stopLoading
{
    if (FrameLoader* frameLoader = [self _frameLoader])
        frameLoader->stopForUserCancel();
}

- (void)reload
{
    [self _frameLoader]->reload();
}

- (WebFrame *)findFrameNamed:(NSString *)name
{
    Frame* coreFrame = core(self);
    if (!coreFrame)
        return nil;
    return kit(coreFrame->tree()->find(name));
}

- (WebFrame *)parentFrame
{
    Frame* coreFrame = core(self);
    if (!coreFrame)
        return nil;
    return [[kit(coreFrame->tree()->parent()) retain] autorelease];
}

- (NSArray *)childFrames
{
    Frame* coreFrame = core(self);
    if (!coreFrame)
        return [NSArray array];
    NSMutableArray *children = [NSMutableArray arrayWithCapacity:coreFrame->tree()->childCount()];
    for (Frame* child = coreFrame->tree()->firstChild(); child; child = child->tree()->nextSibling())
        [children addObject:kit(child)];
    return children;
}

- (WebScriptObject *)windowObject
{
    Frame* coreFrame = core(self);
    if (!coreFrame)
        return 0;
    return coreFrame->windowScriptObject();
}

- (JSGlobalContextRef)globalContext
{
    Frame* coreFrame = core(self);
    if (!coreFrame)
        return 0;
    return toGlobalRef(coreFrame->scriptProxy()->globalObject()->globalExec());
}

@end
