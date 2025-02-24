#include "vimtextedit.h"
#include <QKeyCombination>
#include <QKeyEvent>
#include <QApplication>
#include <QDebug>
#include <QStatusBar>
#include <QFlags>

#ifdef Q_OS_WIN
    #include <windows.h>
#elif defined(Q_OS_LINUX)
    #include <X11/XKBlib.h>
    #include <X11/Xlib.h>
#endif

// #define Move Move



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

void VimTextEdit::Move(QKeyCombination key)
{
    static const QHash<QKeyCombination, QTextCursor::MoveOperation> keyToMoveDir = {
        {Qt::Key_H, MoveDir::Left},
        {Qt::Key_J, MoveDir::Down},
        {Qt::Key_K, MoveDir::Up},
        {Qt::Key_L, MoveDir::Right},
        {Qt::Key_W, MoveDir::NextWord}
    };

    // default cases
    if (keyToMoveDir.contains(key))
    {
        moveCursor(keyToMoveDir.value(key));
        return;
    }

    QChar currCh = currChar(), nextCh = currChar(1);
    switch (key)
    {
        case Qt::Key_E:
            if ( currCh.isSpace() || currCh.isNull() || nextCh.isSpace() || nextCh.isNull() )
                moveCursor(MoveDir::NextWord);

            if (nextCh.isNull() || nextCh == '\n')
                moveCursor(MoveDir::NextWord);

            moveCursor(MoveDir::EndOfWord);
            moveCursor(MoveDir::Left);
            break;

        case Qt::Key_B:
                moveCursor(MoveDir::Left);
                moveCursor(MoveDir::StartOfWord);
                break;
    }
}

void VimTextEdit::keyPressEvent(QKeyEvent* event)
{
    QKeyCombination keys = event->keyCombination();
    QString textPressed = event->text();
    // qDebug() << textPressed;

    QChar charPressed = textPressed.isEmpty() ? QChar() : textPressed.at(0);
    Action action = keyToAction.value(keys);


    if (m_mode == Mode::INSERT && action != Navigate)
    {
        QTextEdit::keyPressEvent(event);
        return;
    }
    else if (charPressed.isDigit())
    {
        updateCount(charPressed);
        return;
    }

    switch (action)
    {
        case Action::Move:
            Move(keys);
            break;

        case Action::Navigate:
            resetCapsLock(); // reset capslock state
            updateMode(Mode::NORMAL);
            break;

        case Action::Insert:
            moveCursor(MoveDir::StartOfLine);
            updateMode(INSERT);
            break;

        case Action::append:
            moveCursor(MoveDir::Right);
            updateMode(INSERT);
            break;

        case Action::Append:
            moveCursor(MoveDir::EndOfLine);
            updateMode(INSERT);
            break;
        
        case Action::insert:        updateMode(Mode::INSERT);       break;
        case Action::Visual:        updateMode(Mode::VISUAL);       break;
        case Action::VisualLine:    updateMode(Mode::VISUAL_LINE);  break;
        case Action::VisualBlock:   updateMode(Mode::VISUAL_BLOCK); break;
        
        case Action::insertLine:
            moveCursor(MoveDir::EndOfLine);
            this->insertPlainText("\n");
            updateMode(INSERT);
            break;

        case Action::InsertLine:
            moveCursor(MoveDir::StartOfLine);
            this->insertPlainText("\n");
            moveCursor(MoveDir::Up);
            updateMode(INSERT);
            break;

        // TODO
        case Action::Change:
            change();
            return;
        case Action::Delete:
            
        default:
            break;

    }

    // resetting count after action done
    if (m_count > 1)
        updateCount('1');
}

void VimTextEdit::change()
{
    if (visualMode() || visualLineMode() || visualBlockMode())
    {
        auto tCursor = textCursor();
        tCursor.movePosition(MoveDir::Right, QTextCursor::KeepAnchor);
        tCursor.removeSelectedText();
        updateMode(Mode::INSERT);
    }
    else if (normalMode())
    {
        updateCommand(Action::Change);
    }

}

void VimTextEdit::updateCount(QChar countChar)
{
    if (m_count != countChar.digitValue())
    {
        m_count = countChar.digitValue();
        emit countChanged("count = " + QString(countChar));
    }
}

void VimTextEdit::updateCommand(Action command)
{
    if (command != m_command)
    {
        m_command = command;
        emit commandChanged(commandAsString(command));
    }
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
            // disable cursor blinking in normal mode
            QApplication::setCursorFlashTime(0);

            moveCursor(MoveDir::Left);
            setCursorWidth(CURSOR_WIDTH_NORMAL);
            break;

        case Mode::INSERT:
            // enable cursor blinking in insert mode
            QApplication::setCursorFlashTime(cursorFlashTime);

            setCursorWidth(CURSOR_WIDTH_INSERT);
            break;

        case Mode::VISUAL:
        case Mode::VISUAL_LINE:
        case Mode::VISUAL_BLOCK:
            setCursorWidth(CURSOR_WIDTH_NORMAL);
            break;
    }

    
    // emit mode changed signal
    emit modeChanged("-- " + modeAsString(m_mode) + " --");
}

void VimTextEdit::moveCursor(MoveDir operation)
{
    auto c = textCursor();
    c.movePosition(
        operation,
        (m_mode == VISUAL ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor),
        m_count // TODO: m_count
    );
    setTextCursor(c);
}

QString VimTextEdit::modeAsString(Mode mode)
{
    switch (mode)
    {
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

QString VimTextEdit::commandAsString(Action command)
{
    switch (command)
    {
        case Action::Delete:
            return "d";
        case Action::Change:
            return "c";
        default:
            return "No Command";
    }
}



/**
 * @brief current char that cursor is on (the character which will be deleted if backspace clicked)
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
