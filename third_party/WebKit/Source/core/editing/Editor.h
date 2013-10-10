/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef Editor_h
#define Editor_h

#include "core/dom/ClipboardAccessPolicy.h"
#include "core/dom/DocumentMarker.h"
#include "core/editing/EditAction.h"
#include "core/editing/EditingBehavior.h"
#include "core/editing/FindOptions.h"
#include "core/editing/FrameSelection.h"
#include "core/editing/TextIterator.h"
#include "core/editing/VisibleSelection.h"
#include "core/editing/WritingDirection.h"
#include "core/page/FrameDestructionObserver.h"
#include "core/platform/chromium/PasteMode.h"

namespace WebCore {

class Clipboard;
class CompositeEditCommand;
class EditCommand;
class EditCommandComposition;
class EditorClient;
class EditorInternalCommand;
class Frame;
class HTMLElement;
class HitTestResult;
class KillRing;
class Pasteboard;
class SharedBuffer;
class SimpleFontData;
class SpellChecker;
class StylePropertySet;
class Text;
class TextEvent;

enum EditorCommandSource { CommandFromMenuOrKeyBinding, CommandFromDOM, CommandFromDOMWithUserInterface };
enum EditorParagraphSeparator { EditorParagraphSeparatorIsDiv, EditorParagraphSeparatorIsP };

class Editor : public FrameDestructionObserver {
public:
    explicit Editor(Frame&);
    ~Editor();

    EditorClient& client() const;

    Frame& frame() const { return *m_frame; }

    CompositeEditCommand* lastEditCommand() { return m_lastEditCommand.get(); }

    void handleKeyboardEvent(KeyboardEvent*);
    bool handleTextEvent(TextEvent*);

    bool canEdit() const;
    bool canEditRichly() const;

    bool canDHTMLCut();
    bool canDHTMLCopy();
    bool canDHTMLPaste();

    bool canCut() const;
    bool canCopy() const;
    bool canPaste() const;
    bool canDelete() const;
    bool canSmartCopyOrDelete();

    void cut();
    void copy();
    void paste();
    void pasteAsPlainText();
    void performDelete();

    void copyImage(const HitTestResult&);

    void indent();
    void outdent();
    void transpose();

    bool shouldDeleteRange(Range*) const;

    void respondToChangedContents(const VisibleSelection& endingSelection);

    bool selectionStartHasStyle(CSSPropertyID, const String& value) const;
    TriState selectionHasStyle(CSSPropertyID, const String& value) const;
    String selectionStartCSSPropertyValue(CSSPropertyID);

    TriState selectionUnorderedListState() const;
    TriState selectionOrderedListState() const;
    PassRefPtr<Node> insertOrderedList();
    PassRefPtr<Node> insertUnorderedList();
    bool canIncreaseSelectionListLevel();
    bool canDecreaseSelectionListLevel();
    PassRefPtr<Node> increaseSelectionListLevel();
    PassRefPtr<Node> increaseSelectionListLevelOrdered();
    PassRefPtr<Node> increaseSelectionListLevelUnordered();
    void decreaseSelectionListLevel();

    void removeFormattingAndStyle();

    void clearLastEditCommand();

    bool deleteWithDirection(SelectionDirection, TextGranularity, bool killRing, bool isTypingAction);
    void deleteSelectionWithSmartDelete(bool smartDelete);

    Node* removedAnchor() const { return m_removedAnchor.get(); }
    void setRemovedAnchor(PassRefPtr<Node> n) { m_removedAnchor = n; }

    void applyStyle(StylePropertySet*, EditAction = EditActionUnspecified);
    void applyParagraphStyle(StylePropertySet*, EditAction = EditActionUnspecified);
    void applyStyleToSelection(StylePropertySet*, EditAction);
    void applyParagraphStyleToSelection(StylePropertySet*, EditAction);

    void appliedEditing(PassRefPtr<CompositeEditCommand>);
    void unappliedEditing(PassRefPtr<EditCommandComposition>);
    void reappliedEditing(PassRefPtr<EditCommandComposition>);

    void setShouldStyleWithCSS(bool flag) { m_shouldStyleWithCSS = flag; }
    bool shouldStyleWithCSS() const { return m_shouldStyleWithCSS; }

    class Command {
    public:
        Command();
        Command(const EditorInternalCommand*, EditorCommandSource, PassRefPtr<Frame>);

        bool execute(const String& parameter = String(), Event* triggeringEvent = 0) const;
        bool execute(Event* triggeringEvent) const;

        bool isSupported() const;
        bool isEnabled(Event* triggeringEvent = 0) const;

        TriState state(Event* triggeringEvent = 0) const;
        String value(Event* triggeringEvent = 0) const;

        bool isTextInsertion() const;

    private:
        const EditorInternalCommand* m_command;
        EditorCommandSource m_source;
        RefPtr<Frame> m_frame;
    };
    Command command(const String& commandName); // Command source is CommandFromMenuOrKeyBinding.
    Command command(const String& commandName, EditorCommandSource);
    static bool commandIsSupportedFromMenuOrKeyBinding(const String& commandName); // Works without a frame.

    bool insertText(const String&, Event* triggeringEvent);
    bool insertTextWithoutSendingTextEvent(const String&, bool selectInsertedText, TextEvent* triggeringEvent);
    bool insertLineBreak();
    bool insertParagraphSeparator();

    bool isOverwriteModeEnabled() const { return m_overwriteModeEnabled; }
    void toggleOverwriteModeEnabled();

    void clearUndoRedoOperations();
    bool canUndo();
    void undo();
    bool canRedo();
    void redo();

    void didBeginEditing(Element*);

    void setBaseWritingDirection(WritingDirection);

    // smartInsertDeleteEnabled and selectTrailingWhitespaceEnabled are
    // mutually exclusive, meaning that enabling one will disable the other.
    bool smartInsertDeleteEnabled();
    bool isSelectTrailingWhitespaceEnabled();

    bool setSelectionOffsets(int selectionStart, int selectionEnd);

    bool preventRevealSelection() const { return m_preventRevealSelection; }

    void setStartNewKillRingSequence(bool);

    PassRefPtr<Range> rangeForPoint(const IntPoint& windowPoint);

    void clear();

    VisibleSelection selectionForCommand(Event*);

    KillRing* killRing() const { return m_killRing.get(); }

    EditingBehavior behavior() const;

    PassRefPtr<Range> selectedRange();

    void addToKillRing(Range*, bool prepend);

    void pasteAsFragment(PassRefPtr<DocumentFragment>, bool smartReplace, bool matchStyle);
    void pasteAsPlainText(const String&, bool smartReplace);

    Node* findEventTargetFrom(const VisibleSelection&) const;

    bool findString(const String&, FindOptions);
    // FIXME: Switch callers over to the FindOptions version and retire this one.
    bool findString(const String&, bool forward, bool caseFlag, bool wrapFlag, bool startInSelection);

    PassRefPtr<Range> rangeOfString(const String&, Range*, FindOptions);
    PassRefPtr<Range> findStringAndScrollToVisible(const String&, Range*, FindOptions);

    const VisibleSelection& mark() const; // Mark, to be used as emacs uses it.
    void setMark(const VisibleSelection&);

    void computeAndSetTypingStyle(StylePropertySet* , EditAction = EditActionUnspecified);

    IntRect firstRectForRange(Range*) const;

    void respondToChangedSelection(const VisibleSelection& oldSelection, FrameSelection::SetSelectionOptions);

    bool markedTextMatchesAreHighlighted() const;
    void setMarkedTextMatchesAreHighlighted(bool);

    void textAreaOrTextFieldDidBeginEditing(Element*);
    void textFieldDidEndEditing(Element*);
    void textDidChangeInTextField(Element*);
    bool doTextFieldCommandFromEvent(Element*, KeyboardEvent*);

    void replaceSelectionWithFragment(PassRefPtr<DocumentFragment>, bool selectReplacement, bool smartReplace, bool matchStyle);
    void replaceSelectionWithText(const String&, bool selectReplacement, bool smartReplace);

    void simplifyMarkup(Node* startNode, Node* endNode);

    EditorParagraphSeparator defaultParagraphSeparator() const { return m_defaultParagraphSeparator; }
    void setDefaultParagraphSeparator(EditorParagraphSeparator separator) { m_defaultParagraphSeparator = separator; }

    class RevealSelectionScope {
        WTF_MAKE_NONCOPYABLE(RevealSelectionScope);
    public:
        RevealSelectionScope(Editor*);
        ~RevealSelectionScope();
    private:
        Editor* m_editor;
    };
    friend class RevealSelectionScope;

private:
    RefPtr<CompositeEditCommand> m_lastEditCommand;
    RefPtr<Node> m_removedAnchor;
    int m_preventRevealSelection;
    bool m_shouldStartNewKillRingSequence;
    bool m_shouldStyleWithCSS;
    OwnPtr<KillRing> m_killRing;
    VisibleSelection m_mark;
    bool m_areMarkedTextMatchesHighlighted;
    EditorParagraphSeparator m_defaultParagraphSeparator;
    bool m_overwriteModeEnabled;

    bool canDeleteRange(Range*) const;

    bool tryDHTMLCopy();
    bool tryDHTMLCut();
    bool tryDHTMLPaste(PasteMode);

    bool canSmartReplaceWithPasteboard(Pasteboard*);
    void pasteAsPlainTextWithPasteboard(Pasteboard*);
    void pasteWithPasteboard(Pasteboard*, bool allowPlainText);
    bool dispatchCPPEvent(const AtomicString&, ClipboardAccessPolicy, PasteMode = AllMimeTypes);

    void revealSelectionAfterEditingOperation(const ScrollAlignment& = ScrollAlignment::alignCenterIfNeeded, RevealExtentOption = DoNotRevealExtent);
    void changeSelectionAfterCommand(const VisibleSelection& newSelection, FrameSelection::SetSelectionOptions);
    void notifyComponentsOnChangedSelection(const VisibleSelection& oldSelection, FrameSelection::SetSelectionOptions);

    Node* findEventTargetFromSelection() const;

    SpellChecker& spellChecker() const;
    bool isContinuousSpellCheckingEnabled() const;
};

inline void Editor::setStartNewKillRingSequence(bool flag)
{
    m_shouldStartNewKillRingSequence = flag;
}

inline const VisibleSelection& Editor::mark() const
{
    return m_mark;
}

inline void Editor::setMark(const VisibleSelection& selection)
{
    m_mark = selection;
}

inline bool Editor::markedTextMatchesAreHighlighted() const
{
    return m_areMarkedTextMatchesHighlighted;
}


} // namespace WebCore

#endif // Editor_h
