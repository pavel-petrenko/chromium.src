/*
 * Copyright (C) 2009, 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ContextMenuClientImpl.h"

#include "CSSPropertyNames.h"
#include "HTMLNames.h"
#include "WebContextMenuData.h"
#include "WebDataSourceImpl.h"
#include "WebFormElement.h"
#include "WebFrameImpl.h"
#include "WebMenuItemInfo.h"
#include "WebPlugin.h"
#include "WebPluginContainerImpl.h"
#include "WebSearchableFormData.h"
#include "WebSpellCheckClient.h"
#include "WebViewClient.h"
#include "WebViewImpl.h"
#include "bindings/v8/ExceptionStatePlaceholder.h"
#include "core/css/CSSStyleDeclaration.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentMarkerController.h"
#include "core/editing/Editor.h"
#include "core/editing/SpellChecker.h"
#include "core/history/HistoryItem.h"
#include "core/html/HTMLFormElement.h"
#include "core/html/HTMLInputElement.h"
#include "core/html/HTMLMediaElement.h"
#include "core/html/HTMLPlugInImageElement.h"
#include "core/html/HTMLVideoElement.h"
#include "core/html/MediaError.h"
#include "core/loader/DocumentLoader.h"
#include "core/loader/FrameLoader.h"
#include "core/page/ContextMenuController.h"
#include "core/page/EventHandler.h"
#include "core/frame/FrameView.h"
#include "core/page/Page.h"
#include "core/page/Settings.h"
#include "core/platform/ContextMenu.h"
#include "core/rendering/HitTestResult.h"
#include "core/rendering/RenderWidget.h"
#include "platform/Widget.h"
#include "platform/text/TextBreakIterator.h"
#include "public/platform/WebPoint.h"
#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"
#include "public/platform/WebURLResponse.h"
#include "public/platform/WebVector.h"
#include "weborigin/KURL.h"
#include "wtf/text/WTFString.h"

using namespace WebCore;

namespace WebKit {

// Figure out the URL of a page or subframe. Returns |page_type| as the type,
// which indicates page or subframe, or ContextNodeType::NONE if the URL could not
// be determined for some reason.
static WebURL urlFromFrame(Frame* frame)
{
    if (frame) {
        DocumentLoader* dl = frame->loader()->documentLoader();
        if (dl) {
            WebDataSource* ds = WebDataSourceImpl::fromDocumentLoader(dl);
            if (ds)
                return ds->hasUnreachableURL() ? ds->unreachableURL() : ds->request().url();
        }
    }
    return WebURL();
}

// Helper function to determine whether text is a single word.
static bool isASingleWord(const String& text)
{
    TextBreakIterator* it = wordBreakIterator(text, 0, text.length());
    return it && it->next() == static_cast<int>(text.length());
}

// Helper function to get misspelled word on which context menu
// is to be invoked. This function also sets the word on which context menu
// has been invoked to be the selected word, as required. This function changes
// the selection only when there were no selected characters on OS X.
static String selectMisspelledWord(Frame* selectedFrame)
{
    // First select from selectedText to check for multiple word selection.
    String misspelledWord = selectedFrame->selectedText().stripWhiteSpace();

    // If some texts were already selected, we don't change the selection.
    if (!misspelledWord.isEmpty()) {
        // Don't provide suggestions for multiple words.
        if (!isASingleWord(misspelledWord))
            return String();
        return misspelledWord;
    }

    // Selection is empty, so change the selection to the word under the cursor.
    HitTestResult hitTestResult = selectedFrame->eventHandler()->
        hitTestResultAtPoint(selectedFrame->page()->contextMenuController().hitTestResult().pointInInnerNodeFrame());
    Node* innerNode = hitTestResult.innerNode();
    VisiblePosition pos(innerNode->renderer()->positionForPoint(
        hitTestResult.localPoint()));

    if (pos.isNull())
        return misspelledWord; // It is empty.

    WebFrameImpl::selectWordAroundPosition(selectedFrame, pos);
    misspelledWord = selectedFrame->selectedText().stripWhiteSpace();

#if OS(MACOSX)
    // If misspelled word is still empty, then that portion should not be
    // selected. Set the selection to that position only, and do not expand.
    if (misspelledWord.isEmpty())
        selectedFrame->selection().setSelection(VisibleSelection(pos));
#else
    // On non-Mac, right-click should not make a range selection in any case.
    selectedFrame->selection().setSelection(VisibleSelection(pos));
#endif
    return misspelledWord;
}

static bool IsWhiteSpaceOrPunctuation(UChar c)
{
    return isSpaceOrNewline(c) || WTF::Unicode::isPunct(c);
}

static String selectMisspellingAsync(Frame* selectedFrame, DocumentMarker& marker)
{
    VisibleSelection selection = selectedFrame->selection().selection();
    if (!selection.isCaretOrRange())
        return String();

    // Caret and range selections always return valid normalized ranges.
    RefPtr<Range> selectionRange = selection.toNormalizedRange();
    Vector<DocumentMarker*> markers = selectedFrame->document()->markers()->markersInRange(selectionRange.get(), DocumentMarker::MisspellingMarkers());
    if (markers.size() != 1)
        return String();
    marker = *markers[0];

    // Cloning a range fails only for invalid ranges.
    RefPtr<Range> markerRange = selectionRange->cloneRange(ASSERT_NO_EXCEPTION);
    markerRange->setStart(markerRange->startContainer(), marker.startOffset());
    markerRange->setEnd(markerRange->endContainer(), marker.endOffset());

    if (markerRange->text().stripWhiteSpace(&IsWhiteSpaceOrPunctuation) != selectionRange->text().stripWhiteSpace(&IsWhiteSpaceOrPunctuation))
        return String();

    return markerRange->text();
}

void ContextMenuClientImpl::showContextMenu(const WebCore::ContextMenu* defaultMenu)
{
    // Displaying the context menu in this function is a big hack as we don't
    // have context, i.e. whether this is being invoked via a script or in
    // response to user input (Mouse event WM_RBUTTONDOWN,
    // Keyboard events KeyVK_APPS, Shift+F10). Check if this is being invoked
    // in response to the above input events before popping up the context menu.
    if (!m_webView->contextMenuAllowed())
        return;

    HitTestResult r = m_webView->page()->contextMenuController().hitTestResult();
    Frame* selectedFrame = r.innerNodeFrame();

    WebContextMenuData data;
    data.mousePosition = selectedFrame->view()->contentsToWindow(r.roundedPointInInnerNodeFrame());

    // Compute edit flags.
    data.editFlags = WebContextMenuData::CanDoNone;
    if (m_webView->focusedWebCoreFrame()->editor().canUndo())
        data.editFlags |= WebContextMenuData::CanUndo;
    if (m_webView->focusedWebCoreFrame()->editor().canRedo())
        data.editFlags |= WebContextMenuData::CanRedo;
    if (m_webView->focusedWebCoreFrame()->editor().canCut())
        data.editFlags |= WebContextMenuData::CanCut;
    if (m_webView->focusedWebCoreFrame()->editor().canCopy())
        data.editFlags |= WebContextMenuData::CanCopy;
    if (m_webView->focusedWebCoreFrame()->editor().canPaste())
        data.editFlags |= WebContextMenuData::CanPaste;
    if (m_webView->focusedWebCoreFrame()->editor().canDelete())
        data.editFlags |= WebContextMenuData::CanDelete;
    // We can always select all...
    data.editFlags |= WebContextMenuData::CanSelectAll;
    data.editFlags |= WebContextMenuData::CanTranslate;

    // Links, Images, Media tags, and Image/Media-Links take preference over
    // all else.
    data.linkURL = r.absoluteLinkURL();

    if (!r.absoluteImageURL().isEmpty()) {
        data.srcURL = r.absoluteImageURL();
        data.mediaType = WebContextMenuData::MediaTypeImage;
        data.mediaFlags |= WebContextMenuData::MediaCanPrint;
    } else if (!r.absoluteMediaURL().isEmpty()) {
        data.srcURL = r.absoluteMediaURL();

        // We know that if absoluteMediaURL() is not empty, then this
        // is a media element.
        HTMLMediaElement* mediaElement = toHTMLMediaElement(r.innerNonSharedNode());
        if (isHTMLVideoElement(mediaElement))
            data.mediaType = WebContextMenuData::MediaTypeVideo;
        else if (mediaElement->hasTagName(HTMLNames::audioTag))
            data.mediaType = WebContextMenuData::MediaTypeAudio;

        if (mediaElement->error())
            data.mediaFlags |= WebContextMenuData::MediaInError;
        if (mediaElement->paused())
            data.mediaFlags |= WebContextMenuData::MediaPaused;
        if (mediaElement->muted())
            data.mediaFlags |= WebContextMenuData::MediaMuted;
        if (mediaElement->loop())
            data.mediaFlags |= WebContextMenuData::MediaLoop;
        if (mediaElement->supportsSave())
            data.mediaFlags |= WebContextMenuData::MediaCanSave;
        if (mediaElement->hasAudio())
            data.mediaFlags |= WebContextMenuData::MediaHasAudio;
        if (mediaElement->hasVideo())
            data.mediaFlags |= WebContextMenuData::MediaHasVideo;
        if (mediaElement->controls())
            data.mediaFlags |= WebContextMenuData::MediaControls;
    } else if (r.innerNonSharedNode()->hasTagName(HTMLNames::objectTag)
               || r.innerNonSharedNode()->hasTagName(HTMLNames::embedTag)) {
        RenderObject* object = r.innerNonSharedNode()->renderer();
        if (object && object->isWidget()) {
            Widget* widget = toRenderWidget(object)->widget();
            if (widget && widget->isPluginContainer()) {
                data.mediaType = WebContextMenuData::MediaTypePlugin;
                WebPluginContainerImpl* plugin = toPluginContainerImpl(widget);
                WebString text = plugin->plugin()->selectionAsText();
                if (!text.isEmpty()) {
                    data.selectedText = text;
                    data.editFlags |= WebContextMenuData::CanCopy;
                }
                data.editFlags &= ~WebContextMenuData::CanTranslate;
                data.linkURL = plugin->plugin()->linkAtPosition(data.mousePosition);
                if (plugin->plugin()->supportsPaginatedPrint())
                    data.mediaFlags |= WebContextMenuData::MediaCanPrint;

                HTMLPlugInImageElement* pluginElement = toHTMLPlugInImageElement(r.innerNonSharedNode());
                data.srcURL = pluginElement->document().completeURL(pluginElement->url());
                data.mediaFlags |= WebContextMenuData::MediaCanSave;

                // Add context menu commands that are supported by the plugin.
                if (plugin->plugin()->canRotateView())
                    data.mediaFlags |= WebContextMenuData::MediaCanRotate;
            }
        }
    }

    // An image can to be null for many reasons, like being blocked, no image
    // data received from server yet.
    data.hasImageContents =
        (data.mediaType == WebContextMenuData::MediaTypeImage)
        && r.image() && !(r.image()->isNull());

    // If it's not a link, an image, a media element, or an image/media link,
    // show a selection menu or a more generic page menu.
    if (selectedFrame->document()->loader())
        data.frameEncoding = selectedFrame->document()->encodingName();

    // Send the frame and page URLs in any case.
    data.pageURL = urlFromFrame(m_webView->mainFrameImpl()->frame());
    if (selectedFrame != m_webView->mainFrameImpl()->frame()) {
        data.frameURL = urlFromFrame(selectedFrame);
        RefPtr<HistoryItem> historyItem = selectedFrame->loader()->history()->currentItem();
        if (historyItem)
            data.frameHistoryItem = WebHistoryItem(historyItem);
    }

    if (r.isSelected()) {
        if (!r.innerNonSharedNode()->hasTagName(HTMLNames::inputTag) || !toHTMLInputElement(r.innerNonSharedNode())->isPasswordField())
            data.selectedText = selectedFrame->selectedText().stripWhiteSpace();
    }

    if (r.isContentEditable()) {
        data.isEditable = true;
#if ENABLE(INPUT_SPEECH)
        if (r.innerNonSharedNode()->hasTagName(HTMLNames::inputTag))
            data.isSpeechInputEnabled = toHTMLInputElement(r.innerNonSharedNode())->isSpeechEnabled();
#endif
        // When Chrome enables asynchronous spellchecking, its spellchecker adds spelling markers to misspelled
        // words and attaches suggestions to these markers in the background. Therefore, when a user right-clicks
        // a mouse on a word, Chrome just needs to find a spelling marker on the word instead of spellchecking it.
        if (selectedFrame->settings() && selectedFrame->settings()->asynchronousSpellCheckingEnabled()) {
            DocumentMarker marker;
            data.misspelledWord = selectMisspellingAsync(selectedFrame, marker);
            data.misspellingHash = marker.hash();
            if (marker.description().length()) {
                Vector<String> suggestions;
                marker.description().split('\n', suggestions);
                data.dictionarySuggestions = suggestions;
            } else if (m_webView->spellCheckClient()) {
                int misspelledOffset, misspelledLength;
                m_webView->spellCheckClient()->spellCheck(data.misspelledWord, misspelledOffset, misspelledLength, &data.dictionarySuggestions);
            }
        } else {
            data.isSpellCheckingEnabled =
                m_webView->focusedWebCoreFrame()->spellChecker().isContinuousSpellCheckingEnabled();
            // Spellchecking might be enabled for the field, but could be disabled on the node.
            if (m_webView->focusedWebCoreFrame()->spellChecker().isSpellCheckingEnabledInFocusedNode()) {
                data.misspelledWord = selectMisspelledWord(selectedFrame);
                if (m_webView->spellCheckClient()) {
                    int misspelledOffset, misspelledLength;
                    m_webView->spellCheckClient()->spellCheck(
                        data.misspelledWord, misspelledOffset, misspelledLength,
                        &data.dictionarySuggestions);
                    if (!misspelledLength)
                        data.misspelledWord.reset();
                }
            }
        }
        HTMLFormElement* form = selectedFrame->selection().currentForm();
        if (form && r.innerNonSharedNode()->hasTagName(HTMLNames::inputTag)) {
            HTMLInputElement* selectedElement = toHTMLInputElement(r.innerNonSharedNode());
            if (selectedElement) {
                WebSearchableFormData ws = WebSearchableFormData(WebFormElement(form), WebInputElement(selectedElement));
                if (ws.url().isValid())
                    data.keywordURL = ws.url();
            }
        }
    }

#if OS(MACOSX)
    if (selectedFrame->editor().selectionHasStyle(CSSPropertyDirection, "ltr") != FalseTriState)
        data.writingDirectionLeftToRight |= WebContextMenuData::CheckableMenuItemChecked;
    if (selectedFrame->editor().selectionHasStyle(CSSPropertyDirection, "rtl") != FalseTriState)
        data.writingDirectionRightToLeft |= WebContextMenuData::CheckableMenuItemChecked;
#endif // OS(MACOSX)

    // Now retrieve the security info.
    DocumentLoader* dl = selectedFrame->loader()->documentLoader();
    WebDataSource* ds = WebDataSourceImpl::fromDocumentLoader(dl);
    if (ds)
        data.securityInfo = ds->response().securityInfo();

    data.referrerPolicy = static_cast<WebReferrerPolicy>(selectedFrame->document()->referrerPolicy());

    // Filter out custom menu elements and add them into the data.
    populateCustomMenuItems(defaultMenu, &data);

    data.node = r.innerNonSharedNode();

    WebFrame* selected_web_frame = WebFrameImpl::fromFrame(selectedFrame);
    if (m_webView->client())
        m_webView->client()->showContextMenu(selected_web_frame, data);
}

void ContextMenuClientImpl::clearContextMenu()
{
    if (m_webView->client())
        m_webView->client()->clearContextMenu();
}

static void populateSubMenuItems(const Vector<ContextMenuItem>& inputMenu, WebVector<WebMenuItemInfo>& subMenuItems)
{
    Vector<WebMenuItemInfo> subItems;
    for (size_t i = 0; i < inputMenu.size(); ++i) {
        const ContextMenuItem* inputItem = &inputMenu.at(i);
        if (inputItem->action() < ContextMenuItemBaseCustomTag || inputItem->action() > ContextMenuItemLastCustomTag)
            continue;

        WebMenuItemInfo outputItem;
        outputItem.label = inputItem->title();
        outputItem.enabled = inputItem->enabled();
        outputItem.checked = inputItem->checked();
        outputItem.action = static_cast<unsigned>(inputItem->action() - ContextMenuItemBaseCustomTag);
        switch (inputItem->type()) {
        case ActionType:
            outputItem.type = WebMenuItemInfo::Option;
            break;
        case CheckableActionType:
            outputItem.type = WebMenuItemInfo::CheckableOption;
            break;
        case SeparatorType:
            outputItem.type = WebMenuItemInfo::Separator;
            break;
        case SubmenuType:
            outputItem.type = WebMenuItemInfo::SubMenu;
            populateSubMenuItems(inputItem->subMenuItems(), outputItem.subMenuItems);
            break;
        }
        subItems.append(outputItem);
    }

    WebVector<WebMenuItemInfo> outputItems(subItems.size());
    for (size_t i = 0; i < subItems.size(); ++i)
        outputItems[i] = subItems[i];
    subMenuItems.swap(outputItems);
}

void ContextMenuClientImpl::populateCustomMenuItems(const WebCore::ContextMenu* defaultMenu, WebContextMenuData* data)
{
    populateSubMenuItems(defaultMenu->items(), data->customItems);
}

} // namespace WebKit
