#include "vimtextedit.h"
#include <QKeyCombination>
#include <QKeyEvent>
#include <QApplication>
#include <QDebug>
#include <QStatusBar>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QFlags>

#ifdef Q_OS_WIN
    #include <windows.h>
#elif defined(Q_OS_LINUX)
    #include <X11/XKBlib.h>
    #include <X11/Xlib.h>
#endif



void resetCapsLock()
{
#ifdef Q_OS_WIN
// Windows: If Caps Lock is on, toggle it off
    if ((GetKeyState(VK_CAPITAL) & 0x0001)!=0)
    {
        keybd_event( VK_CAPITAL, 0x3a, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0 );
        keybd_event( VK_CAPITAL, 0x3a, KEYEVENTF_EXTENDEDKEY, 0 );
    }


#elif defined(Q_OS_LINUX)
    // Linux (X11): Disable Caps Lock if it was toggled
    Display *display = XOpenDisplay(nullptr);
    if (display) {
        unsigned int state;
        XkbGetIndicatorState(display, XkbUseCoreKbd, &state);
        if (state & 1) { // If Caps Lock is ON, turn it OFF
            XkbLockModifiers(display, XkbUseCoreKbd, LockMask, 0);
        }
        XCloseDisplay(display);
    }
#endif
}

VimTextEdit::VimTextEdit(QWidget* parent):
 QTextEdit(parent)
{
    // setting font
    setFontFamily("Cascadia Code");
    setFontPointSize(14);

    // setting cursor width in normal mode
    QFontMetrics metrics(font());
    CURSOR_WIDTH_NORMAL = metrics.averageCharWidth() + 4;

    updateMode(NORMAL);
}

void VimTextEdit::keyPressEvent(QKeyEvent* event)
{
    auto key = event->key();
    auto text = event->text();
    auto modifiers = event->modifiers();

    if (key == Qt::Key_CapsLock)
    {
        resetCapsLock(); // reset capslock state
        updateMode(NORMAL);
        return;
    }
    else if (m_mode == INSERT)
    {
        QTextEdit::keyPressEvent(event);
        return;
    }
    


    if (key == Qt::Key_V)
    {
        if (modifiers & Qt::ControlModifier)
            updateMode(Mode::VISUAL_BLOCK);
        else if (modifiers & Qt::ShiftModifier)
            updateMode(Mode::VISUAL_LINE);
        else
            updateMode(Mode::VISUAL);
        return;
    }

    auto currCursor = textCursor();
    QChar currCh = currChar(0), nextCh = currChar(1);
    switch (key)
    {
        // Movements
        case Qt::Key_H:
            moveCursor(QTextCursor::MoveOperation::Left);
            return;
        case Qt::Key_J:
            moveCursor(QTextCursor::MoveOperation::Down);
            return;
        case Qt::Key_K:
            moveCursor(QTextCursor::MoveOperation::Up);
            return;
        case Qt::Key_L:
            if (!currCursor.atEnd())
                moveCursor(QTextCursor::MoveOperation::Right);
            return;

        case Qt::Key_W:
            moveCursor(QTextCursor::MoveOperation::NextWord);
            return;
        case Qt::Key_E:
            if ( currCh.isSpace() || currCh.isNull() || nextCh.isSpace() || nextCh.isNull() )
                moveCursor(QTextCursor::NextWord);
            if (nextCh.isNull() || nextCh == '\n')
                moveCursor(QTextCursor::NextWord);

            moveCursor(QTextCursor::MoveOperation::EndOfWord);
            moveCursor(QTextCursor::Left);
            return;
        case Qt::Key_B:
            // if ()
            moveCursor(QTextCursor::MoveOperation::Left);
            moveCursor(QTextCursor::MoveOperation::StartOfWord);
            return;
    }
// sdklfj dklasjf   kladfj

    if (text == "i")
    {
        // Insert (on current position)
        updateMode(INSERT);
    }

    else if (text == "I")
    {
        // Insert (to start of line)
        moveCursor(QTextCursor::MoveOperation::StartOfLine);
        updateMode(INSERT);
    }

    else if (text == "a")
    {
        // Append (after current char)
        moveCursor(QTextCursor::MoveOperation::Right);
        updateMode(INSERT);
    }
        
    else if (text == "A")
    {
        // Append (to end of line)
        moveCursor(QTextCursor::MoveOperation::EndOfLine);
        updateMode(INSERT);
    }

    else if (text == "o")
    {
        // insert line down
        moveCursor(QTextCursor::MoveOperation::EndOfLine);
        this->insertPlainText("\n");
        // moveCursor(QTextCursor::MoveOperation::Down);
        updateMode(INSERT);
    }

    else if (text == "O")
    {
        // insert line up
        moveCursor(QTextCursor::MoveOperation::StartOfLine);
        this->insertPlainText("\n");
        moveCursor(QTextCursor::MoveOperation::Up);
        updateMode(INSERT);
    }


    // else if (text == "a")

    // else if (text == "A")

    // else if (text == "o")

    // else if (text == "O")

}

void VimTextEdit::updateMode(Mode mode)
{
    if (mode == m_mode)
        return;
    m_mode = mode;

    static int cursorFlashTime = QApplication::cursorFlashTime();
    
    switch (m_mode)
    {
        case Mode::NORMAL:
            QApplication::setCursorFlashTime(0); // disable cursor blinking
            moveCursor(QTextCursor::MoveOperation::Left);
            setCursorWidth(CURSOR_WIDTH_NORMAL);
            break;

        case Mode::INSERT:
            QApplication::setCursorFlashTime(cursorFlashTime); // enable cursor blinking
            setCursorWidth(CURSOR_WIDTH_INSERT);
            break;

        case Mode::VISUAL:
        case Mode::VISUAL_LINE:
        case Mode::VISUAL_BLOCK:
            setCursorWidth(CURSOR_WIDTH_NORMAL);
            break;
    }

    
    emit modeChanged("-- " + modeAsString(m_mode) + " --");
}

void VimTextEdit::moveCursor(QTextCursor::MoveOperation operation, uint count)
{
    auto c = textCursor();
    c.movePosition(
        operation,
        (m_mode == VISUAL ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor),
        count
    );
    setTextCursor(c);
}

QString VimTextEdit::modeAsString(Mode mode)
{
    switch (mode)
    {
        case Mode::NORMAL:
            return "NORMAL";
        case Mode::INSERT:
            return "INSERT";
        case Mode::VISUAL:
            return "VISUAL";
        case Mode::VISUAL_LINE:
            return "VISUAL LINE";
        case Mode::VISUAL_BLOCK:
            return "VISUAL BLOCK";
        default:
            return "NORMAL";
    }
}


/**
 * @brief current char that cursor is on
 * 
 * @param offset 1 right char, -1 left char, 0 current char (default) and so on..
 * @return QChar 
 */
QChar VimTextEdit::currChar(int offset) const
{
    int pos = textCursor().position() + offset;
    QString text = this->toPlainText();

    if (pos < text.length() && pos >= 0)
        return this->toPlainText().at(pos);
    else
        return QChar();
}
