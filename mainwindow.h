#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    inline bool isDocumentUntitled() const { return m_filename.isEmpty(); }
    inline bool isDocumentSaved() const { return m_saved; }
    bool isDocumentEmpty() const;

private:
    void saveDocument();
    void saveAsDocument();
    void openDocument();
    void newDocument();
    void search();
    int askToSave();
    inline void setFilename(const QString& filename)
    {
        if (m_filename != filename)
        {
            m_filename = filename;
            setWindowTitle("Vimmy - " + 
                            (m_filename.isEmpty() ? "untitled" : m_filename) +
                            "[*]");
        }
    }



    inline void setSavedStatus(bool saved) {
        if (saved == m_saved)
            return;
        m_saved = saved;

        setWindowModified(!m_saved);
        qDebug() << "isWinModified: " << isWindowModified();
        // if (!m_saved)
            // setWindowModified(true);
    }

private:
    Ui::MainWindow *ui;
    bool m_saved = true;
    QString m_filename;
};
#endif // MAINWINDOW_H
