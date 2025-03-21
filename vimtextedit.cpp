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


const QHash<QKeyCombination, Action> VimTextEdit::keyToAction = {
    // Moving
    {QKeyCombination(Qt::Key_H), Action::Move},
    {QKeyCombination(Qt::Key_J), Action::Move},
    {QKeyCombination(Qt::Key_K), Action::Move},
    {QKeyCombination(Qt::Key_L), Action::Move},
    {QKeyCombination(Qt::Key_W), Action::Move},

    {QKeyCombination(Qt::Key_B), Action::Move},
    {QKeyCombination(Qt::Key_E), Action::Move},

    {QKeyCombination(Qt::Key_C), Action::Change},
    {QKeyCombination(Qt::Key_D), Action::Delete},
    {QKeyCombination(Qt::Key_X), CharDelete},

    // Switching Modes
    {QKeyCombination(Qt::Key_CapsLock), Navigate},

    {QKeyCombination(Qt::Key_V), Visual},
    {QKeyCombination(Qt::Key_V | Qt::ShiftModifier), VisualLine},
    {QKeyCombination(Qt::Key_V | Qt::ControlModifier), VisualBlock},

    {QKeyCombination(Qt::Key_I), insert},
    {QKeyCombination(Qt::Key_I | Qt::ShiftModifier), Insert},

    {QKeyCombination(Qt::Key_O), insertLine},
    {QKeyCombination(Qt::Key_O | Qt::ShiftModifier), InsertLine},

    {QKeyCombination(Qt::Key_A), Action::append},
    {QKeyCombination(Qt::Key_A | Qt::ShiftModifier), Append}
};


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

MoveDir keyToMoveDir(QKeyCombination key) 
{
    switch (key)
    {
        case Qt::Key_H: return MoveDir::Left;
        case Qt::Key_J: return MoveDir::Down;
        case Qt::Key_K: return MoveDir::Up;
        case Qt::Key_L: return MoveDir::Right;
        case Qt::Key_W: return MoveDir::NextWord;
        default:        return MoveDir::NoMove;
    }
}


void VimTextEdit::Move(QKeyCombination key, QTextCursor::MoveMode mode)
{
    auto moveDir = keyToMoveDir(key);
    if (moveDir != MoveDir::NoMove)
    {
        moveCursor(moveDir, mode);
        return;
    }

    QChar currCh = currChar(), nextCh = currChar(1);
    switch (key)
    {
        case Qt::Key_E:
            if ( currCh.isSpace() || currCh.isNull() || nextCh.isSpace() || nextCh.isNull() )
                moveCursor(MoveDir::NextWord, mode);

            if (nextCh.isNull() || nextCh == '\n')
                moveCursor(MoveDir::NextWord, mode);

            moveCursor(MoveDir::EndOfWord, mode);
            moveCursor(MoveDir::Left, mode);
            break;

        case Qt::Key_B:
            moveCursor(MoveDir::Left, mode);
            moveCursor(MoveDir::StartOfWord, mode);
            break;
    }
}

void VimTextEdit::keyPressEvent(QKeyEvent* event)
{
    QKeyCombination keys = event->keyCombination();
    QString textEntered = event->text();

    QChar charPressed = textEntered.isEmpty() ? QChar() : textEntered.at(0);
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
    else
    {
        executeAction(action, keys);
        // resetting count after action done
        if (m_count > 1)
            updateCount('1');
    }
}

void VimTextEdit::executeAction(Action action, QKeyCombination keys)
{
    switch (action)
    {
        case Action::Move:
            if (m_command == Action::Change)
                Change(keys);
            else if (m_command == Action::Delete)
                Delete(keys);
            else
                Move(keys);
            break;

        case Action::Navigate:
            resetCapsLock(); // reset capslock state
            updateCommand(Action::None);
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

        case Action::Change:
            if (normalMode())
                updateCommand(Action::Change);
            else
                Change(keys);
            break;

        case Action::Delete:
            if (normalMode())
                updateCommand(Action::Delete);
            else
                Delete(keys);
            break;

        case Action::CharDelete:
            Delete(Qt::Key_L);
            break;
            
        default:
            break;
    }
}

void VimTextEdit::Change(QKeyCombination key)
{
    Delete(key);
    updateMode(Mode::INSERT);
}

void VimTextEdit::Delete(QKeyCombination key)
{
    if (normalMode())
        Move(key, QTextCursor::KeepAnchor);

    auto tCursor = textCursor();
    if (!normalMode())
        tCursor.movePosition(MoveDir::Right, QTextCursor::KeepAnchor);
    tCursor.removeSelectedText();
    setTextCursor(tCursor);

    updateCommand(Action::None);
    updateMode(Mode::NORMAL);
}



void VimTextEdit::updateCount(QChar countChar)
{
    if (m_count != countChar.digitValue())
    {
        m_count = countChar.digitValue();
        emit countChanged("count: " + QString(countChar));
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

void VimTextEdit::moveCursor(MoveDir operation, QTextCursor::MoveMode mode)
{
    if (visualMode() || visualLineMode() || visualBlockMode())
        mode = QTextCursor::KeepAnchor;
    auto c = textCursor();
    c.movePosition(operation, mode, m_count);
    setTextCursor(c);
}

QString VimTextEdit::modeAsString(Mode mode)
{
    switch (mode)
    {
        case Mode::INSERT:       return "INSERT";
        case Mode::VISUAL:       return "VISUAL";
        case Mode::VISUAL_LINE:  return "VISUAL LINE";
        case Mode::VISUAL_BLOCK: return "VISUAL BLOCK";
        default:                 return "NORMAL";
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
