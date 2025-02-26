#ifndef VIMTEXTEDIT_H
#define VIMTEXTEDIT_H

#include <QTextEdit>
#include <QWidget>
#include <QString>
#include <QKeyEvent>
#include <QChar>
#include <QHash>
#include <initializer_list>

using MoveDir = QTextCursor::MoveOperation;

enum Mode {NORMAL = 0, INSERT, VISUAL, VISUAL_LINE, VISUAL_BLOCK};
enum Action
{
    None = 0,
    Move,   // move operation (hjkl, w, b, e)
    Change, // change operation (Delete + Insert)
    CharDelete, // delete a char (x)
    Delete, // delete operation

    Navigate,    // Change Mode To Normal
    Visual,      // Change Mode To Visual
    VisualLine,  // Change Mode To Visual Line
    VisualBlock, // Change Mode To Visual Block

    insert, // Change Mode To Insert (curr pos)
    Insert, // Change Mode To Insert (beg. of line)

    insertLine, // Insert New Line Below Current Line
    InsertLine, // Insert New Line Above Current Line

    append, // Change Mode To Insert (curr pos + 1)
    Append, // Change Mode To Insert (eof line)
};

const QHash<QKeyCombination, Action> keyToAction = {
    // Moving
    {QKeyCombination(Qt::Key_H), Move},
    {QKeyCombination(Qt::Key_J), Move},
    {QKeyCombination(Qt::Key_K), Move},
    {QKeyCombination(Qt::Key_L), Move},
    {QKeyCombination(Qt::Key_W), Move},

    {QKeyCombination(Qt::Key_B), Move},
    {QKeyCombination(Qt::Key_E), Move},

    {QKeyCombination(Qt::Key_C), Change},
    {QKeyCombination(Qt::Key_D), Delete},
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

    {QKeyCombination(Qt::Key_A), append},
    {QKeyCombination(Qt::Key_A | Qt::ShiftModifier), Append}
};


class VimTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit VimTextEdit(QWidget* parent = nullptr);
    
    inline bool isEmpty() const { return toPlainText().isEmpty(); }

signals:
    void modeChanged(const QString& modeStr);
    void countChanged(const QString& countStr);
    void commandChanged(const QString& commandStr);

private:
    // OVERRIDDEN
    void keyPressEvent(QKeyEvent* event) override;
    // ------------


    void updateMode(Mode mode);
    void updateCount(QChar countChar);
    void updateCommand(Action command);

    inline bool normalMode() const {return m_mode == Mode::NORMAL;}
    inline bool visualMode() const {return m_mode == Mode::VISUAL;}
    inline bool visualLineMode() const {return m_mode == Mode::VISUAL_LINE;}
    inline bool visualBlockMode() const {return m_mode == Mode::VISUAL_BLOCK;}

    void moveCursor(QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

    QChar currChar(int offset = 0) const;

    void Move(QKeyCombination key, QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);
    void change(QKeyCombination key);
    void Delete(QKeyCombination key);



    static inline bool isSwitchMode(Action action) {return action < Action::Navigate;}
    static QString modeAsString(Mode mode);
    static QString commandAsString(Action command);
private:
    Mode m_mode = INSERT;

    int CURSOR_WIDTH_INSERT = 1;
    int CURSOR_WIDTH_NORMAL = 6;

    // command count
    qint16 m_count = 1;
    Action m_command = Action::None;
};

#endif // VIMTEXTEDIT_H
