#ifndef DEEPREADER_H
#define DEEPREADER_H

#include <QMainWindow>
#include <poppler-qt5.h>

namespace Ui {
class DeepReader;
}

class DeepReader : public QMainWindow
{
    Q_OBJECT

public:
    explicit DeepReader(QWidget *parent = 0);
    ~DeepReader();
    // loads new page based on current value of pageCounter and displays it
    void showPage();
    // extracts text from pdf page
    void pageText(); // I'm not sure what this should return yet
    // determines whether notes are good enough to move to the next page
    bool goodNotes();

private slots:
    void on_actionOpen_triggered();

    void on_previous_clicked();

    void on_next_clicked();

    void on_lineEdit_returnPressed();

    void on_actionSave_As_triggered();

    void on_actionOpen_Text_File_triggered();

    void on_bold_clicked();

    void on_italic_clicked();

    void on_underline_clicked();

    void change_font_size();

    void on_zoom_out_clicked();

    void on_zoom_in_clicked();

    void on_start_clicked();

private:
    Ui::DeepReader *ui;
    Poppler::Document *doc;
    int pageCounter;
    int zoom;
    bool studySession;
    int startPage;
    int endPage;
    QStringList words;
    // so that I can ignore words in notes
    // from previous pages
    int previousText;
};

#endif // DEEPREADER_H
