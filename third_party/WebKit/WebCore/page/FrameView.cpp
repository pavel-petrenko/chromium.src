/* This file is part of the KDE project
 *
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999 Lars Knoll <knoll@kde.org>
 *                     1999 Antti Koivisto <koivisto@kde.org>
 *                     2000 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2004 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "config.h"
#include "FrameView.h"

#include "Frame.h"
#include "SelectionController.h"
#include "khtml_events.h"

#include "html/html_documentimpl.h"
#include "html/html_inlineimpl.h"
#include "HTMLInputElementImpl.h"
#include "rendering/render_arena.h"
#include "rendering/render_object.h"
#include "rendering/render_canvas.h"
#include "rendering/render_style.h"
#include "rendering/render_replaced.h"
#include "rendering/render_line.h"
#include "RenderText.h"
#include "xml/dom2_eventsimpl.h"
#include "xml/EventNames.h"
#include "css/cssstyleselector.h"
#include "helper.h"
#include "khtml_settings.h"
#include "MacFrame.h"

#include <kcursor.h>
#include <kstandarddirs.h>

#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qvariant.h>

#include <kimageio.h>
#include <assert.h>
#include <kdebug.h>

#include "KWQAccObjectCache.h"

#include "CachedImage.h"

// #define INSTRUMENT_LAYOUT_SCHEDULING 1

using namespace DOM;
using namespace EventNames;
using namespace HTMLNames;
using namespace khtml;

class KHTMLViewPrivate {
public:
    KHTMLViewPrivate()
    {
        repaintRects = 0;
        underMouse = 0;
        clickNode = 0;
        reset();
        layoutTimerId = 0;
        delayedLayout = false;
        mousePressed = false;
        doFullRepaint = true;
        isTransparent = false;
        vmode = hmode = QScrollView::Auto;
        firstLayout = true;
        needToInitScrollBars = true;
    }
    ~KHTMLViewPrivate()
    {
        if (underMouse)
	    underMouse->deref();
        if (clickNode)
	    clickNode->deref();
        delete repaintRects;
    }
    void reset()
    {
        if (underMouse)
	    underMouse->deref();
	underMouse = 0;
        linkPressed = false;
        useSlowRepaints = false;
        dragTarget = 0;
	borderTouched = false;
        scrollBarMoved = false;
        ignoreWheelEvents = false;
	borderX = 30;
	borderY = 30;
        prevMouseX = -1;
        prevMouseY = -1;
	clickCount = 0;
        if (clickNode)
	    clickNode->deref();
        clickNode = 0;
	scrollingSelf = false;
	layoutTimerId = 0;
        delayedLayout = false;
        mousePressed = false;
        doFullRepaint = true;
        layoutSchedulingEnabled = true;
        layoutSuppressed = false;
        layoutCount = 0;
        firstLayout = true;
        if (repaintRects)
            repaintRects->clear();
    }

    NodeImpl *underMouse;

    bool borderTouched:1;
    bool borderStart:1;
    bool scrollBarMoved:1;
    bool doFullRepaint:1;
    
    QScrollView::ScrollBarMode vmode;
    QScrollView::ScrollBarMode hmode;
    bool linkPressed;
    bool useSlowRepaints;
    bool ignoreWheelEvents;

    int borderX, borderY;
    int clickCount;
    NodeImpl *clickNode;

    int prevMouseX, prevMouseY;
    bool scrollingSelf;
    int layoutTimerId;
    bool delayedLayout;
    
    bool layoutSchedulingEnabled;
    bool layoutSuppressed;
    int layoutCount;

    bool firstLayout;
    bool needToInitScrollBars;
    bool mousePressed;
    bool isTransparent;
    
    // Used by objects during layout to communicate repaints that need to take place only
    // after all layout has been completed.
    QPtrList<RenderObject::RepaintInfo>* repaintRects;
    
    RefPtr<NodeImpl> dragTarget;
};

KHTMLView::KHTMLView( Frame *frame, QWidget *parent, const char *name)
    : QScrollView( parent, name, WResizeNoErase | WRepaintNoErase | WPaintUnclipped ),
      _refCount(1)
{
    m_medium = "screen";

    m_frame = frame;
    m_frame->ref();
    d = new KHTMLViewPrivate;

    connect(this, SIGNAL(contentsMoving(int, int)), this, SLOT(slotScrollBarMoved()));

    KImageIO::registerFormats();

    init();

    viewport()->show();
}

KHTMLView::~KHTMLView()
{
    resetScrollBars();

    assert(_refCount == 0);

    if (m_frame)
    {
        //WABA: Is this Ok? Do I need to deref it as well?
        //Does this need to be done somewhere else?
        DocumentImpl *doc = m_frame->xmlDocImpl();
        if (doc)
            doc->detach();

        m_frame->deref();
    }

    delete d; d = 0;
}

void KHTMLView::clearPart()
{
    if (m_frame){
        m_frame->deref();
        m_frame = 0;
    }
}

void KHTMLView::resetScrollBars()
{
    // Reset the document's scrollbars back to our defaults before we yield the floor.
    d->firstLayout = true;
    suppressScrollBars(true);
    QScrollView::setVScrollBarMode(d->vmode);
    QScrollView::setHScrollBarMode(d->hmode);
    suppressScrollBars(false);
}

void KHTMLView::init()
{

    setFocusPolicy(QWidget::StrongFocus);
    viewport()->setFocusPolicy( QWidget::WheelFocus );
    viewport()->setFocusProxy(this);

    _marginWidth = -1; // undefined
    _marginHeight = -1;
    _width = 0;
    _height = 0;
}

void KHTMLView::clear()
{
//    viewport()->erase();

    setStaticBackground(false);
    
    // FIXME: 6498 Should just be able to call m_frame->selection().clear()
    m_frame->setSelection(SelectionController());

    d->reset();

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (d->layoutTimerId && m_frame->xmlDocImpl() && !m_frame->xmlDocImpl()->ownerElement())
        printf("Killing the layout timer from a clear at %d\n", m_frame->xmlDocImpl()->elapsedTime());
#endif
    
    killTimers();
    emit cleared();

    suppressScrollBars(true);
}

void KHTMLView::resizeEvent(QResizeEvent* e)
{
    if (m_frame && m_frame->xmlDocImpl())
        m_frame->xmlDocImpl()->dispatchWindowEvent(EventNames::resizeEvent, false, false);
}

void KHTMLView::initScrollBars()
{
    if (!d->needToInitScrollBars)
        return;
    d->needToInitScrollBars = false;
    setScrollBarsMode(hScrollBarMode());
}

void KHTMLView::setMarginWidth(int w)
{
    // make it update the rendering area when set
    _marginWidth = w;
}

void KHTMLView::setMarginHeight(int h)
{
    // make it update the rendering area when set
    _marginHeight = h;
}

void KHTMLView::adjustViewSize()
{
    if( m_frame->xmlDocImpl() ) {
        DocumentImpl *document = m_frame->xmlDocImpl();

        RenderCanvas* root = static_cast<RenderCanvas *>(document->renderer());
        if ( !root )
            return;
        
        int docw = root->docWidth();
        int doch = root->docHeight();
    
        resizeContents(docw, doch);
    }
}

void KHTMLView::applyOverflowToViewport(RenderObject* o, ScrollBarMode& hMode, ScrollBarMode& vMode)
{
    // Handle the overflow:hidden/scroll case for the body/html elements.  WinIE treats
    // overflow:hidden and overflow:scroll on <body> as applying to the document's
    // scrollbars.  The CSS2.1 draft states that HTML UAs should use the <html> or <body> element and XML/XHTML UAs should
    // use the root element.
    switch(o->style()->overflow()) {
        case OHIDDEN:
            hMode = vMode = AlwaysOff;
            break;
        case OSCROLL:
            hMode = vMode = AlwaysOn;
            break;
        case OAUTO:
	    hMode = vMode = Auto;
            break;
        default:
            // Don't set it at all.
            ;
    }
}

bool KHTMLView::inLayout() const
{
    return d->layoutSuppressed;
}

int KHTMLView::layoutCount() const
{
    return d->layoutCount;
}

bool KHTMLView::needsFullRepaint() const
{
    return d->doFullRepaint;
}

void KHTMLView::addRepaintInfo(RenderObject* o, const IntRect& r)
{
    if (!d->repaintRects) {
        d->repaintRects = new QPtrList<RenderObject::RepaintInfo>;
        d->repaintRects->setAutoDelete(true);
    }
    
    d->repaintRects->append(new RenderObject::RepaintInfo(o, r));
}

void KHTMLView::layout()
{
    if (d->layoutSuppressed)
        return;
    
    killTimer(d->layoutTimerId);
    d->layoutTimerId = 0;
    d->delayedLayout = false;

    if (!m_frame) {
        // FIXME: Do we need to set _width here?
        // FIXME: Should we set _height here too?
        _width = visibleWidth();
        return;
    }

    DocumentImpl* document = m_frame->xmlDocImpl();
    if (!document) {
        // FIXME: Should we set _height here too?
        _width = visibleWidth();
        return;
    }

    d->layoutSchedulingEnabled = false;

    // Always ensure our style info is up-to-date.  This can happen in situations where
    // the layout beats any sort of style recalc update that needs to occur.
    if (document->hasChangedChild())
        document->recalcStyle();

    RenderCanvas* root = static_cast<RenderCanvas*>(document->renderer());
    if (!root) {
        // FIXME: Do we need to set _width or _height here?
        d->layoutSchedulingEnabled = true;
        return;
    }

    ScrollBarMode hMode = d->hmode;
    ScrollBarMode vMode = d->vmode;
    
    RenderObject* rootRenderer = document->documentElement() ? document->documentElement()->renderer() : 0;
    if (document->isHTMLDocument()) {
        NodeImpl *body = static_cast<HTMLDocumentImpl*>(document)->body();
        if (body && body->renderer()) {
            if (body->hasTagName(framesetTag)) {
                body->renderer()->setNeedsLayout(true);
                vMode = AlwaysOff;
                hMode = AlwaysOff;
            }
            else if (body->hasTagName(bodyTag)) {
                RenderObject* o = (rootRenderer->style()->overflow() == OVISIBLE) ? body->renderer() : rootRenderer;
                applyOverflowToViewport(o, hMode, vMode); // Only applies to HTML UAs, not to XML/XHTML UAs
            }
        }
    }
    else if (rootRenderer)
        applyOverflowToViewport(rootRenderer, hMode, vMode); // XML/XHTML UAs use the root element.

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (d->firstLayout && !document->ownerElement())
        printf("Elapsed time before first layout: %d\n", document->elapsedTime());
#endif

    d->doFullRepaint = d->firstLayout || root->printingMode();
    if (d->repaintRects)
        d->repaintRects->clear();

    // Now set our scrollbar state for the layout.
    ScrollBarMode currentHMode = hScrollBarMode();
    ScrollBarMode currentVMode = vScrollBarMode();

    bool didFirstLayout = false;
    if (d->firstLayout || (hMode != currentHMode || vMode != currentVMode)) {
        suppressScrollBars(true);
        if (d->firstLayout) {
            d->firstLayout = false;
            didFirstLayout = true;
            
            // Set the initial vMode to AlwaysOn if we're auto.
            if (vMode == Auto)
                QScrollView::setVScrollBarMode(AlwaysOn); // This causes a vertical scrollbar to appear.
            // Set the initial hMode to AlwaysOff if we're auto.
            if (hMode == Auto)
                QScrollView::setHScrollBarMode(AlwaysOff); // This causes a horizontal scrollbar to disappear.
        }
        
        if (hMode == vMode)
            QScrollView::setScrollBarsMode(hMode);
        else {
            QScrollView::setHScrollBarMode(hMode);
            QScrollView::setVScrollBarMode(vMode);
        }

        suppressScrollBars(false, true);
    }

    int oldHeight = _height;
    int oldWidth = _width;
    
    _height = visibleHeight();
    _width = visibleWidth();

    if (oldHeight != _height || oldWidth != _width)
        d->doFullRepaint = true;
    
    RenderLayer* layer = root->layer();
     
    if (!d->doFullRepaint) {
        layer->computeRepaintRects();
        root->repaintObjectsBeforeLayout();
    }

    root->layout();

    m_frame->invalidateSelection();
   
    d->layoutSchedulingEnabled=true;
    d->layoutSuppressed = false;

    if (!root->printingMode())
        resizeContents(layer->width(), layer->height());

    // Now update the positions of all layers.
    layer->updateLayerPositions(d->doFullRepaint);

    // We update our widget positions right after doing a layout.
    root->updateWidgetPositions();
    
    if (d->repaintRects && !d->repaintRects->isEmpty()) {
        // FIXME: Could optimize this and have objects removed from this list
        // if they ever do full repaints.
        RenderObject::RepaintInfo* r;
        QPtrListIterator<RenderObject::RepaintInfo> it(*d->repaintRects);
        for ( ; (r = it.current()); ++it)
            r->m_object->repaintRectangle(r->m_repaintRect);
        d->repaintRects->clear();
    }
    
    d->layoutCount++;
#if __APPLE__
    if (KWQAccObjectCache::accessibilityEnabled())
        root->document()->getAccObjectCache()->postNotification(root, "AXLayoutComplete");
#endif

    updateDashboardRegions();

    if (root->needsLayout()) {
        //qDebug("needs layout, delaying repaint");
        scheduleRelayout();
        return;
    }
    setStaticBackground(d->useSlowRepaints);

    if (didFirstLayout) {
        m_frame->didFirstLayout();
    }
}

void KHTMLView::updateDashboardRegions()
{
    DocumentImpl* document = m_frame->xmlDocImpl();
    if (document->hasDashboardRegions()) {
        QValueList<DashboardRegionValue> newRegions = document->renderer()->computeDashboardRegions();
        QValueList<DashboardRegionValue> currentRegions = document->dashboardRegions();
	document->setDashboardRegions(newRegions);
	Mac(m_frame)->dashboardRegionsChanged();
    }
}

//
// Event Handling
//
/////////////////

void KHTMLView::viewportMousePressEvent( QMouseEvent *_mouse )
{
    if(!m_frame->xmlDocImpl()) return;

    RefPtr<KHTMLView> protector(this);

    int xm, ym;
    viewportToContents(_mouse->x(), _mouse->y(), xm, ym);

    d->mousePressed = true;

    NodeImpl::MouseEvent mev( _mouse->stateAfter(), NodeImpl::MousePress );
    m_frame->xmlDocImpl()->prepareMouseEvent( false, xm, ym, &mev );

    if (Mac(m_frame)->passSubframeEventToSubframe(mev)) {
        invalidateClick();
        return;
    }

    d->clickCount = _mouse->clickCount();
    if (d->clickNode)
        d->clickNode->deref();
    d->clickNode = mev.innerNode.get();
    if (d->clickNode)
        d->clickNode->ref();

    bool swallowEvent = dispatchMouseEvent(mousedownEvent,mev.innerNode.get(),true,
                                           d->clickCount,_mouse,true,NodeImpl::MousePress);

    if (!swallowEvent) {
	MousePressEvent event( _mouse, xm, ym, mev.url, mev.target, mev.innerNode.get() );
	QApplication::sendEvent( m_frame, &event );
        // Many AK widgets run their own event loops and consume events while the mouse is down.
        // When they finish, currentEvent is the mouseUp that they exited on.  We need to update
        // the khtml state with this mouseUp, which khtml never saw.
        // If this event isn't a mouseUp, we assume that the mouseUp will be coming later.  There
        // is a hole here if the widget consumes the mouseUp and subsequent events.
        if (Mac(m_frame)->lastEventIsMouseUp()) {
            d->mousePressed = false;
        }
    }
}

void KHTMLView::viewportMouseDoubleClickEvent( QMouseEvent *_mouse )
{
    if(!m_frame->xmlDocImpl()) return;

    RefPtr<KHTMLView> protector(this);

    int xm, ym;
    viewportToContents(_mouse->x(), _mouse->y(), xm, ym);

    // We get this instead of a second mouse-up 
    d->mousePressed = false;

    NodeImpl::MouseEvent mev( _mouse->stateAfter(), NodeImpl::MouseDblClick );
    m_frame->xmlDocImpl()->prepareMouseEvent( false, xm, ym, &mev );

    if (Mac(m_frame)->passSubframeEventToSubframe(mev))
        return;

    d->clickCount = _mouse->clickCount();
    bool swallowEvent = dispatchMouseEvent(mouseupEvent,mev.innerNode.get(),true,
                                           d->clickCount,_mouse,false,NodeImpl::MouseRelease);

    if (mev.innerNode == d->clickNode)
        dispatchMouseEvent(clickEvent,mev.innerNode.get(),true,
			   d->clickCount,_mouse,true,NodeImpl::MouseRelease);

    // Qt delivers a release event AND a double click event.
    if (!swallowEvent) {
	MouseReleaseEvent event1( _mouse, xm, ym, mev.url, mev.target, mev.innerNode.get() );
	QApplication::sendEvent( m_frame, &event1 );

	MouseDoubleClickEvent event2( _mouse, xm, ym, mev.url, mev.target, mev.innerNode.get() );
	QApplication::sendEvent( m_frame, &event2 );
    }

    invalidateClick();
}

static bool isSubmitImage(NodeImpl *node)
{
    return node && node->hasTagName(inputTag) && static_cast<HTMLInputElementImpl*>(node)->inputType() == HTMLInputElementImpl::IMAGE;
}

static QCursor selectCursor(const NodeImpl::MouseEvent &event, Frame *frame, bool mousePressed)
{
    // During selection, use an I-beam no matter what we're over
    if (mousePressed && frame->hasSelection())
        return KCursor::ibeamCursor();

    NodeImpl *node = event.innerNode.get();
    RenderObject *renderer = node ? node->renderer() : 0;
    RenderStyle *style = renderer ? renderer->style() : 0;

    if (style && style->cursorImage() && !style->cursorImage()->pixmap().isNull())
        return QCursor(style->cursorImage()->pixmap());

    switch (style ? style->cursor() : CURSOR_AUTO) {
        case CURSOR_AUTO:
            if (!event.url.isNull() || isSubmitImage(node))
                return frame->urlCursor();
            if ((node && node->isContentEditable()) || (renderer && renderer->isText() && renderer->canSelect()))
                return KCursor::ibeamCursor();
            break;
        case CURSOR_CROSS:
            return KCursor::crossCursor();
        case CURSOR_POINTER:
            return frame->urlCursor();
        case CURSOR_MOVE:
            return KCursor::sizeAllCursor();
        case CURSOR_E_RESIZE:
            return KCursor::eastResizeCursor();
        case CURSOR_W_RESIZE:
            return KCursor::westResizeCursor();
        case CURSOR_N_RESIZE:
            return KCursor::northResizeCursor();
        case CURSOR_S_RESIZE:
            return KCursor::southResizeCursor();
        case CURSOR_NE_RESIZE:
            return KCursor::northEastResizeCursor();
        case CURSOR_SW_RESIZE:
            return KCursor::southWestResizeCursor();
        case CURSOR_NW_RESIZE:
            return KCursor::northWestResizeCursor();
        case CURSOR_SE_RESIZE:
            return KCursor::southEastResizeCursor();
        case CURSOR_TEXT:
            return KCursor::ibeamCursor();
        case CURSOR_WAIT:
            return KCursor::waitCursor();
        case CURSOR_HELP:
            return KCursor::whatsThisCursor();
        case CURSOR_DEFAULT:
            break;
    }
    return QCursor();
}

void KHTMLView::viewportMouseMoveEvent( QMouseEvent * _mouse )
{
    // in Radar 3703768 we saw frequent crashes apparently due to the
    // part being null here, which seems impossible, so check for nil
    // but also assert so that we can try to figure this out in debug
    // builds, if it happens.
    assert(m_frame);
    if(!m_frame || !m_frame->xmlDocImpl()) return;

    int xm, ym;
    viewportToContents(_mouse->x(), _mouse->y(), xm, ym);

    // Treat mouse move events while the mouse is pressed as "read-only" in prepareMouseEvent
    // if we are allowed to select.
    // This means that :hover and :active freeze in the state they were in when the mouse
    // was pressed, rather than updating for nodes the mouse moves over as you hold the mouse down.
    NodeImpl::MouseEvent mev( _mouse->stateAfter(), NodeImpl::MouseMove );
    m_frame->xmlDocImpl()->prepareMouseEvent(d->mousePressed && m_frame->mouseDownMayStartSelect(), d->mousePressed, xm, ym, &mev );

    if (!Mac(m_frame)->passSubframeEventToSubframe(mev))
        viewport()->setCursor(selectCursor(mev, m_frame, d->mousePressed));
        
    bool swallowEvent = dispatchMouseEvent(mousemoveEvent,mev.innerNode.get(),false,
                                           0,_mouse,true,NodeImpl::MouseMove);

    // execute the scheduled script. This is to make sure the mouseover events come after the mouseout events
    m_frame->executeScheduledScript();

    d->prevMouseX = xm;
    d->prevMouseY = ym;

    if (!swallowEvent) {
        MouseMoveEvent event(_mouse, xm, ym, mev.url, mev.target, mev.innerNode.get());
        QApplication::sendEvent(m_frame, &event);
    }
}

void KHTMLView::invalidateClick()
{
    d->clickCount = 0;
    if (d->clickNode) {
        d->clickNode->deref();
        d->clickNode = 0;
    }
}

void KHTMLView::viewportMouseReleaseEvent( QMouseEvent * _mouse )
{
    if ( !m_frame->xmlDocImpl() ) return;

    RefPtr<KHTMLView> protector(this);

    int xm, ym;
    viewportToContents(_mouse->x(), _mouse->y(), xm, ym);

    d->mousePressed = false;

    NodeImpl::MouseEvent mev( _mouse->stateAfter(), NodeImpl::MouseRelease );
    m_frame->xmlDocImpl()->prepareMouseEvent( false, xm, ym, &mev );

    if (Mac(m_frame)->passSubframeEventToSubframe(mev))
        return;

    bool swallowEvent = dispatchMouseEvent(mouseupEvent,mev.innerNode.get(),true,
                                           d->clickCount,_mouse,false,NodeImpl::MouseRelease);

    if (d->clickCount > 0 && mev.innerNode == d->clickNode
        )
	dispatchMouseEvent(clickEvent,mev.innerNode.get(),true,
			   d->clickCount,_mouse,true,NodeImpl::MouseRelease);

    if (!swallowEvent) {
	MouseReleaseEvent event( _mouse, xm, ym, mev.url, mev.target, mev.innerNode.get() );
	QApplication::sendEvent( m_frame, &event );
    }

    invalidateClick();
}

void KHTMLView::keyPressEvent( QKeyEvent *_ke )
{
    if (m_frame->xmlDocImpl() && m_frame->xmlDocImpl()->focusNode()) {
        if (m_frame->xmlDocImpl()->focusNode()->dispatchKeyEvent(_ke))
        {
            _ke->accept();
            return;
        }
    }

}

bool KHTMLView::dispatchDragEvent(const AtomicString &eventType, NodeImpl *dragTarget, const IntPoint &loc, ClipboardImpl *clipboard)
{
    int clientX, clientY;
    viewportToContents(loc.x(), loc.y(), clientX, clientY);
    IntPoint screenLoc = viewportToGlobal(loc);
    int screenX = screenLoc.x();
    int screenY = screenLoc.y();
    bool ctrlKey = 0;   // FIXME - set up modifiers, grab from AK or CG
    bool altKey = 0;
    bool shiftKey = 0;
    bool metaKey = 0;
    
    MouseEventImpl *me = new MouseEventImpl(eventType,
                                            true, true, m_frame->xmlDocImpl()->defaultView(),
                                            0, screenX, screenY, clientX, clientY,
                                            ctrlKey, altKey, shiftKey, metaKey,
                                            0, 0, clipboard);
    me->ref();
    int exceptioncode = 0;
    dragTarget->dispatchEvent(me, exceptioncode, true);
    bool accept = me->defaultPrevented();
    me->deref();
    return accept;
}

bool KHTMLView::updateDragAndDrop(const IntPoint &loc, ClipboardImpl *clipboard)
{
    bool accept = false;
    int xm, ym;
    viewportToContents(loc.x(), loc.y(), xm, ym);
    NodeImpl::MouseEvent mev(0, NodeImpl::MouseMove);
    m_frame->xmlDocImpl()->prepareMouseEvent(true, xm, ym, &mev);
    NodeImpl *newTarget = mev.innerNode.get();

    // Drag events should never go to text nodes (following IE, and proper mouseover/out dispatch)
    if (newTarget && newTarget->isTextNode()) {
        newTarget = newTarget->parentNode();
    }

    if (d->dragTarget != newTarget) {
        // note this ordering is explicitly chosen to match WinIE
        if (newTarget) {
            accept = dispatchDragEvent(dragenterEvent, newTarget, loc, clipboard);
        }
        if (d->dragTarget) {
            dispatchDragEvent(dragleaveEvent, d->dragTarget.get(), loc, clipboard);
        }
    } else if (newTarget) {
        accept = dispatchDragEvent(dragoverEvent, newTarget, loc, clipboard);
    }
    d->dragTarget = newTarget;

    return accept;
}

void KHTMLView::cancelDragAndDrop(const IntPoint &loc, ClipboardImpl *clipboard)
{
    if (d->dragTarget) {
        dispatchDragEvent(dragleaveEvent, d->dragTarget.get(), loc, clipboard);
    }
    d->dragTarget = 0;
}

bool KHTMLView::performDragAndDrop(const IntPoint &loc, ClipboardImpl *clipboard)
{
    bool accept = false;
    if (d->dragTarget) {
        accept = dispatchDragEvent(dropEvent, d->dragTarget.get(), loc, clipboard);
    }
    d->dragTarget = 0;
    return accept;
}


NodeImpl *KHTMLView::nodeUnderMouse() const
{
    return d->underMouse;
}

bool KHTMLView::scrollTo(const IntRect &bounds)
{
    d->scrollingSelf = true; // so scroll events get ignored

    int x, y, xe, ye;
    x = bounds.left();
    y = bounds.top();
    xe = bounds.right();
    ye = bounds.bottom();
    
    int deltax;
    int deltay;

    int curHeight = visibleHeight();
    int curWidth = visibleWidth();

    if (ye-y>curHeight-d->borderY)
	ye  = y + curHeight - d->borderY;

    if (xe-x>curWidth-d->borderX)
	xe = x + curWidth - d->borderX;

    // is xpos of target left of the view's border?
    if (x < contentsX() + d->borderX )
            deltax = x - contentsX() - d->borderX;
    // is xpos of target right of the view's right border?
    else if (xe + d->borderX > contentsX() + curWidth)
            deltax = xe + d->borderX - ( contentsX() + curWidth );
    else
        deltax = 0;

    // is ypos of target above upper border?
    if (y < contentsY() + d->borderY)
            deltay = y - contentsY() - d->borderY;
    // is ypos of target below lower border?
    else if (ye + d->borderY > contentsY() + curHeight)
            deltay = ye + d->borderY - ( contentsY() + curHeight );
    else
        deltay = 0;

    int maxx = curWidth-d->borderX;
    int maxy = curHeight-d->borderY;

    int scrollX,scrollY;

    scrollX = deltax > 0 ? (deltax > maxx ? maxx : deltax) : deltax == 0 ? 0 : (deltax>-maxx ? deltax : -maxx);
    scrollY = deltay > 0 ? (deltay > maxy ? maxy : deltay) : deltay == 0 ? 0 : (deltay>-maxy ? deltay : -maxy);

    if (contentsX() + scrollX < 0)
	scrollX = -contentsX();
    else if (contentsWidth() - visibleWidth() - contentsX() < scrollX)
	scrollX = contentsWidth() - visibleWidth() - contentsX();

    if (contentsY() + scrollY < 0)
	scrollY = -contentsY();
    else if (contentsHeight() - visibleHeight() - contentsY() < scrollY)
	scrollY = contentsHeight() - visibleHeight() - contentsY();

    scrollBy(scrollX, scrollY);

    // generate abs(scroll.)
    if (scrollX<0)
        scrollX=-scrollX;
    if (scrollY<0)
        scrollY=-scrollY;

    d->scrollingSelf = false;

    if ( (scrollX!=maxx) && (scrollY!=maxy) )
	return true;
    else return false;

}

void KHTMLView::focusNextPrevNode(bool next)
{
    // Sets the focus node of the document to be the node after (or if next is false, before) the current focus node.
    // Only nodes that are selectable (i.e. for which isSelectable() returns true) are taken into account, and the order
    // used is that specified in the HTML spec (see DocumentImpl::nextFocusNode() and DocumentImpl::previousFocusNode()
    // for details).

    DocumentImpl *doc = m_frame->xmlDocImpl();
    NodeImpl *oldFocusNode = doc->focusNode();
    NodeImpl *newFocusNode;

    // Find the next/previous node from the current one
    if (next)
        newFocusNode = doc->nextFocusNode(oldFocusNode);
    else
        newFocusNode = doc->previousFocusNode(oldFocusNode);

    // If there was previously no focus node and the user has scrolled the document, then instead of picking the first
    // focusable node in the document, use the first one that lies within the visible area (if possible).
    if (!oldFocusNode && newFocusNode && d->scrollBarMoved) {
        bool visible = false;
        NodeImpl *toFocus = newFocusNode;
        while (!visible && toFocus) {
            IntRect focusNodeRect = toFocus->getRect();
            if ((focusNodeRect.left() > contentsX()) && (focusNodeRect.right() < contentsX() + visibleWidth()) &&
                (focusNodeRect.top() > contentsY()) && (focusNodeRect.bottom() < contentsY() + visibleHeight())) {
                // toFocus is visible in the contents area
                visible = true;
            }
            else {
                // toFocus is _not_ visible in the contents area, pick the next node
                if (next)
                    toFocus = doc->nextFocusNode(toFocus);
                else
                    toFocus = doc->previousFocusNode(toFocus);
            }
        }

        if (toFocus)
            newFocusNode = toFocus;
    }

    d->scrollBarMoved = false;

    if (!newFocusNode)
      {
        // No new focus node, scroll to bottom or top depending on next
        if (next)
            scrollTo(IntRect(contentsX()+visibleWidth()/2,contentsHeight(),0,0));
        else
            scrollTo(IntRect(contentsX()+visibleWidth()/2,0,0,0));
    }
    else {
        // EDIT FIXME: if it's an editable element, activate the caret
        // otherwise, hide it
        if (newFocusNode->isContentEditable()) {
            // make caret visible
        } 
        else {
            // hide caret
        }

        // Scroll the view as necessary to ensure that the new focus node is visible
        if (oldFocusNode) {
            if (!scrollTo(newFocusNode->getRect()))
                return;
        }
        else {
            if (doc->renderer()) {
                doc->renderer()->enclosingLayer()->scrollRectToVisible(IntRect(contentsX(), next ? 0: contentsHeight(), 0, 0));
            }
        }
    }
    // Set focus node on the document
    m_frame->xmlDocImpl()->setFocusNode(newFocusNode);
}

void KHTMLView::setMediaType( const QString &medium )
{
    m_medium = medium;
}

QString KHTMLView::mediaType() const
{
    // See if we have an override type.
    QString overrideType = Mac(m_frame)->overrideMediaType();
    if (!overrideType.isNull())
        return overrideType;
    return m_medium;
}


void KHTMLView::useSlowRepaints()
{
    kdDebug(0) << "slow repaints requested" << endl;
    d->useSlowRepaints = true;
    setStaticBackground(true);
}

void KHTMLView::setScrollBarsMode ( ScrollBarMode mode )
{
#ifndef KHTML_NO_SCROLLBARS
    d->vmode = mode;
    d->hmode = mode;
    
    QScrollView::setScrollBarsMode(mode);
#else
    Q_UNUSED( mode );
#endif
}

void KHTMLView::setVScrollBarMode ( ScrollBarMode mode )
{
#ifndef KHTML_NO_SCROLLBARS
    d->vmode = mode;
    QScrollView::setVScrollBarMode(mode);
#else
    Q_UNUSED( mode );
#endif
}

void KHTMLView::setHScrollBarMode ( ScrollBarMode mode )
{
#ifndef KHTML_NO_SCROLLBARS
    d->hmode = mode;
    QScrollView::setHScrollBarMode(mode);
#else
    Q_UNUSED( mode );
#endif
}

void KHTMLView::restoreScrollBar ( )
{
    suppressScrollBars(false);
}


bool KHTMLView::dispatchMouseEvent(const AtomicString &eventType, NodeImpl *targetNode, bool cancelable,
				   int detail,QMouseEvent *_mouse, bool setUnder,
				   int mouseEventType)
{
    // if the target node is a text node, dispatch on the parent node - rdar://4196646
    if (targetNode && targetNode->isTextNode())
        targetNode = targetNode->parentNode();
    if (d->underMouse)
	d->underMouse->deref();
    d->underMouse = targetNode;
    if (d->underMouse)
	d->underMouse->ref();

    // mouseout/mouseover
    if (setUnder) {
        int clientX, clientY;
        viewportToContents(_mouse->x(), _mouse->y(), clientX, clientY);
        if (d->prevMouseX != clientX || d->prevMouseY != clientY) {
            // ### this code sucks. we should save the oldUnder instead of calculating
            // it again. calculating is expensive! (Dirk)
            // Also, there's no guarantee that the old under node is even around any more,
            // so we could be sending a mouseout to a node that never got a mouseover.
            RefPtr<NodeImpl> oldUnder;
            if (d->prevMouseX >= 0 && d->prevMouseY >= 0) {
                NodeImpl::MouseEvent mev( _mouse->stateAfter(), static_cast<NodeImpl::MouseEventType>(mouseEventType));
                m_frame->xmlDocImpl()->prepareMouseEvent( true, d->prevMouseX, d->prevMouseY, &mev );
                oldUnder = mev.innerNode;
                if (oldUnder && oldUnder->isTextNode())
                    oldUnder = oldUnder->parentNode();
            }
            if (oldUnder != targetNode) {
                // send mouseout event to the old node
                if (oldUnder)
                    oldUnder->dispatchMouseEvent(_mouse, mouseoutEvent);
                // send mouseover event to the new node
                if (targetNode)
                    targetNode->dispatchMouseEvent(_mouse, mouseoverEvent);
            }
        }
    }

    bool swallowEvent = false;

    if (targetNode)
        swallowEvent = targetNode->dispatchMouseEvent(_mouse, eventType, detail);
    
    if (!swallowEvent && eventType == mousedownEvent) {
        // Focus should be shifted on mouse down, not on a click.  -dwh
        // Blur current focus node when a link/button is clicked; this
        // is expected by some sites that rely on onChange handlers running
        // from form fields before the button click is processed.
        NodeImpl* node = targetNode;
        for ( ; node && !node->isFocusable(); node = node->parentNode());
        // If focus shift is blocked, we eat the event.  Note we should never clear swallowEvent
        // if the page already set it (e.g., by canceling default behavior).
        if (node && node->isMouseFocusable()) {
            if (!m_frame->xmlDocImpl()->setFocusNode(node))
                swallowEvent = true;
        } else if (!node || !node->focused()) {
            if (!m_frame->xmlDocImpl()->setFocusNode(0))
            swallowEvent = true;
        }
    }

    return swallowEvent;
}

void KHTMLView::setIgnoreWheelEvents( bool e )
{
    d->ignoreWheelEvents = e;
}

void KHTMLView::viewportWheelEvent(QWheelEvent* e)
{
    DocumentImpl *doc = m_frame->xmlDocImpl();
    if (doc) {
        RenderObject *docRenderer = doc->renderer();
        if (docRenderer) {
            int x, y;
            viewportToContents(e->x(), e->y(), x, y);

            RenderObject::NodeInfo hitTestResult(true, false);
            doc->renderer()->layer()->hitTest(hitTestResult, x, y); 
            NodeImpl *node = hitTestResult.innerNode();

           if (Mac(m_frame)->passWheelEventToChildWidget(node)) {
                e->accept();
                return;
            }
            if (node) {
                node->dispatchWheelEvent(e);
                if (e->isAccepted())
                    return;
            }
        }
    }
}

void KHTMLView::focusInEvent( QFocusEvent *e )
{
    m_frame->setCaretVisible();
}

void KHTMLView::focusOutEvent( QFocusEvent *e )
{
    m_frame->setCaretVisible(false);
}

void KHTMLView::slotScrollBarMoved()
{
    if (!d->scrollingSelf)
        d->scrollBarMoved = true;
}

void KHTMLView::repaintRectangle(const IntRect& r, bool immediate)
{
    updateContents(r, immediate);
}

void KHTMLView::timerEvent ( QTimerEvent *e )
{
    if (e->timerId()==d->layoutTimerId) {
#ifdef INSTRUMENT_LAYOUT_SCHEDULING
        if (m_frame->xmlDocImpl() && !m_frame->xmlDocImpl()->ownerElement())
            printf("Layout timer fired at %d\n", m_frame->xmlDocImpl()->elapsedTime());
#endif
        layout();
    }
}

void KHTMLView::scheduleRelayout()
{
    if (!d->layoutSchedulingEnabled)
        return;

    if (!m_frame->xmlDocImpl() || !m_frame->xmlDocImpl()->shouldScheduleLayout())
        return;

    int delay = m_frame->xmlDocImpl()->minimumLayoutDelay();
    if (d->layoutTimerId && d->delayedLayout && !delay)
        unscheduleRelayout();
    if (d->layoutTimerId)
        return;

    d->delayedLayout = delay != 0;

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!m_frame->xmlDocImpl()->ownerElement())
        printf("Scheduling layout for %d\n", delay);
#endif

    d->layoutTimerId = startTimer(delay);
}

bool KHTMLView::layoutPending()
{
    return d->layoutTimerId;
}

bool KHTMLView::haveDelayedLayoutScheduled()
{
    return d->layoutTimerId && d->delayedLayout;
}

void KHTMLView::unscheduleRelayout()
{
    if (!d->layoutTimerId)
        return;

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (m_frame->xmlDocImpl() && !m_frame->xmlDocImpl()->ownerElement())
        printf("Layout timer unscheduled at %d\n", m_frame->xmlDocImpl()->elapsedTime());
#endif
    
    killTimer(d->layoutTimerId);
    d->layoutTimerId = 0;
    d->delayedLayout = false;
}

bool KHTMLView::isTransparent() const
{
    return d->isTransparent;
}

void KHTMLView::setTransparent(bool isTransparent)
{
    d->isTransparent = isTransparent;
}
