/*
 * Copyright (C) 2001 Apple Computer, Inc.  All rights reserved.
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

#include <qcolor.h>
#include <qstring.h>

#include <kwqdebug.h>

QRgb qRgb(int r, int g, int b)
{
    return r << 16 | g << 8 | b;
}


QRgb qRgba(int r, int g, int b, int a)
{
    return a << 24 | r << 16 | g << 8 | b;
}



QColor::QColor()
{
    color = nil;
}


QColor::QColor(int r, int g, int b)
{
    if ( !globals_init )
	initGlobalColors();
    else
        _initialize (r, g, b);
}

QColor::QColor(const QString &name)
{
    color = nil;
    setNamedColor(name);
}

QColor::QColor(const char *name)
{
    color = nil;
    setNamedColor( QString(name) );
}


void QColor::_initialize(int r, int g, int b)
{
    color = [[NSColor colorWithCalibratedRed: ((float)(r)) / (float)255.0
                    green: ((float)(g)) / (float)255.0
                    blue: ((float)(b)) / (float)255.0
                    alpha: 1.0] retain];
}


QColor::~QColor(){
    if (color != nil)
        [color release];
}


QColor::QColor(const QColor &copyFrom)
{
    if (color == copyFrom.color)
        return;
    //if (color != nil)
    //    [color release];
    if (copyFrom.color != nil)
        color = [copyFrom.color retain];
    else
        color = nil;
}

QString QColor::name() const
{
    NSString *name;

    name = [NSString stringWithFormat:@"#%02x%02x%02x", red(), green(), blue()];
    
    return NSSTRING_TO_QSTRING(name);
}

static int hex2int( QChar hexchar )
{
    int v;
    if ( hexchar.isDigit() )
	v = hexchar.digitValue();
    else if ( hexchar >= 'A' && hexchar <= 'F' )
	v = hexchar.cell() - 'A' + 10;
    else if ( hexchar >= 'a' && hexchar <= 'f' )
	v = hexchar.cell() - 'a' + 10;
    else
	v = 0;
    return v;
}


void QColor::setNamedColor(const QString&name)
{

    if ( name.isEmpty() ) {
	setRgb( 0 );
    } 
    else if ( name[0] == '#' || name.length() == 6) {
        int offset = 0;
        if (name[0] == '#') {
            offset = 1;
        }
        const QChar *p = name.unicode() + offset;
        int len = name.length();
        int r, g, b;
        if ( len == 12 ) {
            r = (hex2int(p[0]) << 4) + hex2int(p[1]);
            g = (hex2int(p[4]) << 4) + hex2int(p[5]);
            b = (hex2int(p[8]) << 4) + hex2int(p[9]);
        } else if ( len == 9 ) {
            r = (hex2int(p[0]) << 4) + hex2int(p[1]);
            g = (hex2int(p[3]) << 4) + hex2int(p[4]);
            b = (hex2int(p[6]) << 4) + hex2int(p[7]);
        } else if ( len == 6 ) {
            r = (hex2int(p[0]) << 4) + hex2int(p[1]);
            g = (hex2int(p[2]) << 4) + hex2int(p[3]);
            b = (hex2int(p[4]) << 4) + hex2int(p[5]);
        } else if ( len == 3 ) {
            r = (hex2int(p[0]) << 4) + hex2int(p[0]);
            g = (hex2int(p[1]) << 4) + hex2int(p[1]);
            b = (hex2int(p[2]) << 4) + hex2int(p[2]);
        } else {
            r = g = b = 0;
	}
	setRgb( r, g, b );

    } else {
	if (color != nil)
            [color release];
        color = [NSColor colorWithCatalogName: @"Apple" colorName: QSTRING_TO_NSSTRING(name)];
        if (color == nil) {
            NSLog (@"WARNING %s:%d %s couldn't create color using name %s\n", __FILE__, __LINE__, __FUNCTION__, name.ascii());
            color = [NSColor colorWithCalibratedRed:0 green:0 blue:0 alpha:1];
        }
    }
}


bool QColor::isValid() const
{
    bool result;

    result = TRUE;

    if (color == nil) {
        result = FALSE;
    }

    return result;
}


int QColor::red() const
{
    if (color == nil)
        return 0;
    return (int)([color redComponent] * 255);
}


int QColor::QColor::green() const
{
    if (color == nil)
        return 0;
    return (int)([color greenComponent] * 255);
}

int QColor::blue() const
{
    if (color == nil)
        return 0;
    return (int)([color blueComponent] * 255);
}


QRgb QColor::rgb() const
{
    if (color == nil)
        return 0;
    return qRgb (red(),green(),blue());
}


void QColor::setRgb(int r, int g, int b)
{
    if (color != nil)
        [color release];
    color = [[NSColor colorWithCalibratedRed: ((float)(r)) / (float)255.0
                    green: ((float)(g)) / (float)255.0
                    blue: ((float)(b)) / (float)255.0
                    alpha: 1.0] retain];
}


void QColor::setRgb(int rgb)
{
    if (color != nil)
        [color release];
    color = [[NSColor colorWithCalibratedRed: ((float)(rgb >> 16)) / 255.0
                    green: ((float)(rgb >> 8)) / 255.0
                    blue: ((float)(rgb & 0xff)) / 255.0
                    alpha: 1.0] retain];
}


void QColor::hsv(int *, int *, int *) const
{
    _logNotYetImplemented();
}

QColor QColor::light(int f = 150) const
{
    _logNotYetImplemented();
    return *this;
}


QColor QColor::dark(int f = 200) const
{
    QColor result;
    NSColor *newColor;
    float factor;

    if (f <= 0) {
        result.setRgb(red(), green(), blue());
        return result;
    }
    else if (f < 100) {
        // NOTE: this is actually a lighten operation
        factor = 10000.0f / (float)f; 
        newColor = [color highlightWithLevel:factor];    
    }
    else if (f > 10000) {
        newColor = [color shadowWithLevel:1.0f];    
    }
    else {
        factor = (float)f / 10000.0f;
        newColor = [color shadowWithLevel:factor];    
    }

    result.setRgb(
        (int)([color redComponent] * 255),
        (int)([color greenComponent] * 255),
        (int)([color blueComponent] * 255)
    );
    return result;
}


QColor &QColor::operator=(const QColor &assignFrom)
{
    if ( !globals_init )
	initGlobalColors();
    if (color != assignFrom.color){ 
        if (color != nil)
            [color release];
        if (assignFrom.color != nil)
            color = [assignFrom.color retain];
        else
            color = nil;
    }
    return *this;
}


bool QColor::operator==(const QColor &compareTo) const
{
    return [color isEqual: compareTo.color];
}


bool QColor::operator!=(const QColor &compareTo) const
{
    return !(operator==(compareTo));
}




/*****************************************************************************
  Global colors
 *****************************************************************************/

bool QColor::globals_init = FALSE;		// global color not initialized


static QColor stdcol[19];

QT_STATIC_CONST_IMPL QColor & Qt::color0 = stdcol[0];
QT_STATIC_CONST_IMPL QColor & Qt::color1  = stdcol[1];
QT_STATIC_CONST_IMPL QColor & Qt::black  = stdcol[2];
QT_STATIC_CONST_IMPL QColor & Qt::white = stdcol[3];
QT_STATIC_CONST_IMPL QColor & Qt::darkGray = stdcol[4];
QT_STATIC_CONST_IMPL QColor & Qt::gray = stdcol[5];
QT_STATIC_CONST_IMPL QColor & Qt::lightGray = stdcol[6];
QT_STATIC_CONST_IMPL QColor & Qt::red = stdcol[7];
QT_STATIC_CONST_IMPL QColor & Qt::green = stdcol[8];
QT_STATIC_CONST_IMPL QColor & Qt::blue = stdcol[9];
QT_STATIC_CONST_IMPL QColor & Qt::cyan = stdcol[10];
QT_STATIC_CONST_IMPL QColor & Qt::magenta = stdcol[11];
QT_STATIC_CONST_IMPL QColor & Qt::yellow = stdcol[12];
QT_STATIC_CONST_IMPL QColor & Qt::darkRed = stdcol[13];
QT_STATIC_CONST_IMPL QColor & Qt::darkGreen = stdcol[14];
QT_STATIC_CONST_IMPL QColor & Qt::darkBlue = stdcol[15];
QT_STATIC_CONST_IMPL QColor & Qt::darkCyan = stdcol[16];
QT_STATIC_CONST_IMPL QColor & Qt::darkMagenta = stdcol[17];
QT_STATIC_CONST_IMPL QColor & Qt::darkYellow = stdcol[18];



void QColor::initGlobalColors()
{
    NSAutoreleasePool *colorPool = [[NSAutoreleasePool allocWithZone:NULL] init];
     
    globals_init = TRUE;

    stdcol[ 0].setRgb(255,   255,   255 );
    stdcol[ 1].setRgb(   0,   0,   0 );
    stdcol[ 2].setRgb(   0,   0,   0 );
    stdcol[ 3].setRgb( 255, 255, 255 );
    stdcol[ 4].setRgb( 128, 128, 128 );
    stdcol[ 5].setRgb( 160, 160, 164 );
    stdcol[ 6].setRgb( 192, 192, 192 );
    stdcol[ 7].setRgb( 255,   0,   0 );
    stdcol[ 8].setRgb(   0, 255,   0 );
    stdcol[ 9].setRgb(   0,   0, 255 );
    stdcol[10].setRgb(   0, 255, 255 );
    stdcol[11].setRgb( 255,   0, 255 );
    stdcol[12].setRgb( 255, 255,   0 );
    stdcol[13].setRgb( 128,   0,   0 );
    stdcol[14].setRgb(   0, 128,   0 );
    stdcol[15].setRgb(   0,   0, 128 );
    stdcol[16].setRgb(   0, 128, 128 );
    stdcol[17].setRgb( 128,   0, 128 );
    stdcol[18].setRgb( 128, 128,   0 );
}

