/*
 * Copyright (C) 2003 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef NDEBUG

#include "KWQRenderTreeDebug.h"

#include "htmltags.h"
#include "khtmlview.h"
#include "render_replaced.h"
#include "render_table.h"

#include "KWQKHTMLPart.h"
#include "KWQTextStream.h"

using khtml::RenderLayer;
using khtml::RenderObject;
using khtml::RenderTableCell;
using khtml::RenderWidget;

typedef khtml::RenderLayer::RenderLayerElement RenderLayerElement;

static void writeLayers(QTextStream &ts, const RenderObject &o, int indent = 0);

static QTextStream &operator<<(QTextStream &ts, const QRect &r)
{
    return ts << "(" << r.x() << "," << r.y() << "," << r.width() << "," << r.height() << ")";
}

static QTextStream &operator<<(QTextStream &ts, const RenderObject &o)
{
    ts << o.renderName();
    
    if (o.style() && o.style()->zIndex()) {
        ts << " zI: " << o.style()->zIndex();
    }
    
    if (o.element()) {
        QString tagName(getTagName(o.element()->id()).string());
        if (!tagName.isEmpty()) {
            ts << " {" << tagName << "}";
        }
    }
    
    QRect r(o.xPos(), o.yPos(), o.width(), o.height());
    ts << " " << r;
    
    if (o.isTableCell()) {
        const RenderTableCell &c = static_cast<const RenderTableCell &>(o);
        ts << " [r=" << c.row() << " c=" << c.col() << " rs=" << c.rowSpan() << " cs=" << c.colSpan() << "]";
    }

    return ts;
}

static void writeIndent(QTextStream &ts, int indent)
{
    for (int i = 0; i != indent; ++i) {
        ts << "  ";
    }
}

static void write(QTextStream &ts, const RenderObject &o, int indent = 0)
{
    writeIndent(ts, indent);
    
    ts << o << "\n";
    
    for (RenderObject *child = o.firstChild(); child; child = child->nextSibling()) {
        if (child->layer()) {
            continue;
        }
        write(ts, *child, indent + 1);
    }
    
    if (o.isWidget()) {
        KHTMLView *view = dynamic_cast<KHTMLView *>(static_cast<const RenderWidget &>(o).widget());
        if (view) {
            RenderObject *root = KWQ(view->part())->renderer();
            if (root) {
                writeLayers(ts, *root, indent + 1);
            }
        }
    }
}

static void write(QTextStream &ts, const RenderLayerElement &e, int indent = 0)
{
    RenderLayer &l = *e.layer;
    
    writeIndent(ts, indent);
    
    ts << "RenderLayer";
    
    QRect r(l.xPos(), l.yPos(), l.width(), l.height());
    
    ts << " " << r;
    
    if (r != r.intersect(e.backgroundClipRect)) {
        ts << " backgroundClip" << e.backgroundClipRect;
    }
    if (r != r.intersect(e.clipRect)) {
        ts << " clip" << e.clipRect;
    }
    
    ts << "\n";

    write(ts, *l.renderer(), indent + 1);
}

static void writeLayers(QTextStream &ts, const RenderObject &o, int indent)
{
    QPtrVector<RenderLayerElement> list = o.layer()->elementList();
    for (unsigned i = 0; i != list.count(); ++i) {
        write(ts, *list[i], indent);
    }
}

QString externalRepresentation(const RenderObject *o)
{
    QString s;
    {
        QTextStream ts(&s);
        if (o) {
            writeLayers(ts, *o);
        }
    }
    return s;
}

#endif // NDEBUG
