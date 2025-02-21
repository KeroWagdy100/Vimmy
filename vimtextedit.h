#ifndef VIMTEXTEDIT_H
#define VIMTEXTEDIT_H

#include <QTextEdit>
#include <QWidget>
#include <QString>
#include <QKeyEvent>
#include <QChar>


enum Mode {NORMAL = 0, INSERT, VISUAL, VISUAL_LINE, VISUAL_BLOCK};

class VimTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit VimTextEdit(QWidget* parent = nullptr);
    

signals:
    void modeChanged(const QString& modeStr);

private:
    // OVERRIDDEN
    void keyPressEvent(QKeyEvent* event) override;
    // ------------
    void updateMode(Mode mode);
    void moveCursor(QTextCursor::MoveOperation operation, uint count = 1);
    QChar currChar(int offset = 0) const;



    static QString modeAsString(Mode mode);
private:
    Mode m_mode = INSERT;

    int CURSOR_WIDTH_INSERT = 1;
    int CURSOR_WIDTH_NORMAL = 6;

    qint16 count;
};

#endif // VIMTEXTEDIT_H
