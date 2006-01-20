/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.
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
#ifndef RenderTableCell_H
#define RenderTableCell_H

#include "RenderBlock.h"
#include "RenderTable.h"
#include "RenderTableSection.h"

namespace WebCore {

class RenderTableCell : public RenderBlock
{
public:
    RenderTableCell(DOM::NodeImpl* node);

    virtual void destroy();

    virtual const char *renderName() const { return "RenderTableCell"; }
    virtual bool isTableCell() const { return true; }

    // overrides RenderObject
    virtual bool requiresLayer();

    // ### FIX these two...
    int cellIndex() const { return 0; }
    void setCellIndex(int) { }

    int colSpan() const { return cSpan; }
    void setColSpan(int c) { cSpan = c; }

    int rowSpan() const { return rSpan; }
    void setRowSpan(int r) { rSpan = r; }

    int col() const { return _col; }
    void setCol(int col) { _col = col; }
    int row() const { return _row; }
    void setRow(int r) { _row = r; }

    Length styleOrColWidth();

    // overrides
    virtual void calcMinMaxWidth();
    virtual void calcWidth();
    virtual void setWidth(int width);
    virtual void setStyle(RenderStyle *style);

    int borderLeft() const;
    int borderRight() const;
    int borderTop() const;
    int borderBottom() const;

    CollapsedBorderValue collapsedLeftBorder() const;
    CollapsedBorderValue collapsedRightBorder() const;
    CollapsedBorderValue collapsedTopBorder() const;
    CollapsedBorderValue collapsedBottomBorder() const;
    virtual void collectBorders(QValueList<CollapsedBorderValue>& borderStyles);

    virtual void updateFromElement();

    virtual void layout();
    
    void setCellTopExtra(int p) { _topExtra = p; }
    void setCellBottomExtra(int p) { _bottomExtra = p; }

    virtual void paint(PaintInfo& i, int tx, int ty);

    void paintCollapsedBorder(QPainter* p, int x, int y, int w, int h);
    
    // lie position to outside observers
    virtual int yPos() const { return m_y + _topExtra; }

    virtual void computeAbsoluteRepaintRect(IntRect& r, bool f=false);
    virtual bool absolutePosition(int &xPos, int &yPos, bool f = false);

    virtual short baselinePosition(bool = false) const;

    virtual int borderTopExtra() const { return _topExtra; }
    virtual int borderBottomExtra() const { return _bottomExtra; }

    RenderTable *table() const { return static_cast<RenderTable *>(parent()->parent()->parent()); }
    RenderTableSection *section() const { return static_cast<RenderTableSection *>(parent()->parent()); }

#ifndef NDEBUG
    virtual void dump(QTextStream *stream, QString ind = "") const;
#endif

    virtual IntRect getAbsoluteRepaintRect();
    
protected:
    virtual void paintBoxDecorations(PaintInfo& i, int _tx, int _ty);
    
    int _row;
    int _col;
    int rSpan;
    int cSpan;
    int _topExtra : 31;
    bool nWrap : 1;
    int _bottomExtra : 31;
    bool m_widthChanged : 1;
    
    int m_percentageHeight;
};

}
#endif
