/*
 * Copyright (C) 2002 Apple Computer, Inc.  All rights reserved.
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

#import <WebCoreBridge.h>

#import <KWQKHTMLPartImpl.h>
#import <khtmlview.h>
#import <dom_docimpl.h>
#import <render_object.h>

@implementation WebCoreBridge

- init
{
    [super init];
    
    part = new KHTMLPart;
    part->impl->setBridge(self);
    
    return self;
}

- (void)dealloc
{
    part->deref();
    
    [super dealloc];
}

- (KHTMLPart *)part
{
    return part;
}

- (void)openURL:(NSURL *)URL
{
    part->openURL([[URL absoluteString] cString]);
}

- (void)addData:(NSData *)data withEncoding:(NSString *)encoding
{
    part->impl->slotData(encoding, (const char *)[data bytes], [data length], NO);
}

- (void)closeURL
{
    part->closeURL();
}

- (void)end
{
    part->end();
}

- (void)setURL:(NSURL *)URL
{
    part->impl->setBaseURL([[URL absoluteString] cString]);
}

- (KHTMLView *)createKHTMLViewWithNSView:(NSView *)view
    width:(int)width height:(int)height
    marginWidth:(int)mw marginHeight:(int)mh
{
    // Nasty! Set up the cross references between the KHTMLView and the KHTMLPart.
    KHTMLView *kview = new KHTMLView(part, 0);
    part->impl->setView(kview);

    kview->setView(view);
    if (mw >= 0)
        kview->setMarginWidth(mw);
    if (mh >= 0)
        kview->setMarginHeight(mh);
    kview->resize(width, height);
    
    return kview;
}

- (NSString *)documentTextFromDOM
{
    NSString *string = nil;
    DOM::DocumentImpl *doc = part->xmlDocImpl();
    if (doc) {
        string = [[doc->recursive_toHTML(1).getNSString() copy] autorelease];
    }
    if (string == nil) {
        string = @"";
    }
    return string;
}

- (void)scrollToBaseAnchor
{
    part->impl->gotoBaseAnchor();
}

- (NSString *)selectedText
{
    return [[part->selectedText().getNSString() copy] autorelease];
}

- (void)selectAll
{
    part->selectAll();
}

- (void)reapplyStyles
{
    DOM::DocumentImpl *doc = part->xmlDocImpl();
    if (doc && doc->renderer()) {
        return;
    }
    doc->updateStyleSelector();
}

- (void)forceLayout
{
    DOM::DocumentImpl *doc = part->xmlDocImpl();
    if (doc) {
        khtml::RenderObject *renderer = doc->renderer();
        if (renderer) {
            renderer->setLayouted(false);
        }
    }
    KHTMLView *view = part->impl->getView();
    if (view) {
        view->layout();
    }
}

- (void)_drawRect:(NSRect)rect withPainter: (QPainter *)p
{
    DOM::DocumentImpl *doc = part->xmlDocImpl();
    if (doc) {
        khtml::RenderObject *renderer = doc->renderer();
        if (renderer) {
            renderer->print(p, (int)rect.origin.x, (int)rect.origin.y, (int)rect.size.width, (int)rect.size.height, 0, 0);
        }
    }
}

- (void)drawRect:(NSRect)rect
{
    QPainter p;
    [self _drawRect:rect withPainter: &p];
}

- (void)adjustFrames: (NSRect)rect
{
    // Ick!  khtml sets the frame size during layout and
    // the frame origins during drawing!  So we have to 
    // layout and do a draw with rendering disabled to
    // correclty adjust the frames.
    [self forceLayout];
    QPainter p;
    p.setPaintingDisabled(YES);
    [self _drawRect:rect withPainter: &p];
}



@end
