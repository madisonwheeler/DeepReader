#include <poppler-qt5.h>
#include <QDebug>
#include <QLabel>
#include <QGraphicsScene>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QTextEdit>
#include <cstdio>
#include <ctime>
#include "deepreader.h"
#include "ui_deepreader.h"

DeepReader::DeepReader(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DeepReader)
{
    ui->setupUi(this);
    pageCounter = 0;
    zoom = 0;
    studySession = false;
    previousText = 0; // I should probably set this when I open a document, and reset it at points
    doc = NULL;
    startTime=clock();
    //set timer for 5 min for now (300 sec), need to add so user can put in the time they want
    timeDuration = 300;

    ui->search->setPlaceholderText("Search");
    ui->start_page->setPlaceholderText("Start Page");
    ui->end_page->setPlaceholderText("End page");

    QShortcut *right = new QShortcut(QKeySequence(Qt::Key_Right), this, SLOT(on_next_clicked()));
    QShortcut *left = new QShortcut(QKeySequence(Qt::Key_Left), this, SLOT(on_previous_clicked()));
    // for when zoom is implemented
    QShortcut *in = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus), this, SLOT(on_zoom_in_pdf_clicked()));
    QShortcut *out = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus), this, SLOT(on_zoom_out_pdf_clicked()));

    QShortcut *bullet = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L), this, SLOT(on_action_Bullet_List_triggered()));

    // for text editor
    // QShortcut *text_zoom_in = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus), this, SLOT(on_zoom_in_clicked())); // zoom in doesn't work
    // QShortcut *text_zoom_out = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus), this, SLOT(on_zoom_out_clicked())); // does work
}

DeepReader::~DeepReader()
{
    delete ui;
}

void DeepReader::showPage() {
    ui->lineEdit->setText(QString::number(pageCounter));
    // find poppler page
    Poppler::Page *pdfpage = doc->page(pageCounter);
    if (pdfpage == 0) {
        qDebug() << "you chose page 0, pick again";
    }

    // find dimensions of page
    QSizeF dim = pdfpage->pageSizeF();

    // make page into image
    // we need to figure out page dimensions and zoom
    // QImage image = pdfpage->renderToImage(60+zoom,60+zoom, 0,0, dim.rwidth()+zoom,dim.rheight()+zoom);
    QImage image = pdfpage->renderToImage(72.0 + zoom,72.0 + zoom);
    // args to renderToImage: double xres=72.0, double yres=72.0, int x=-1, int y=-1, int w=-1, int h=-1, Rotation rotate=Rotate0
    if (image.isNull()) {
        qDebug() << "image is null";
    }

    // display image
    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->addPixmap(QPixmap::fromImage(image));
    scene->setSceneRect(QPixmap::fromImage(image).rect());
    ui->mainImage->setScene(scene);
}

void DeepReader::pageText() {
    QString text = doc->page(pageCounter)->text(QRectF(0,0,1000,1000));
    qDebug() << text;
    // I should probably get rid of all the empty strings this makes
    words = text.split(QRegExp("[:;.,]*\\s+"));
    for (int i = 0; i < words.length(); ++i) {
        qDebug() << words[i];
    }
}

bool relevantNotes(QStringList words, QStringList notes, int previousLength){
    // Used set to increase efficiency
    QSet<QString> HundredMostCommonWords({"the","be","to","of","and","a","in","that","have","I","it","for",
            "not","on","with","he","as","you","do","at","this","but","his","by","from","they","we","say",
            "her","she","or","an","will","my","one","all","would","there","their","what","so","up","out",
            "if","about","who","get","which","go","me","when","make","can","like","time","no","just","him",
            "know","take","people","into","year","your","good","some","could","them","see","other","than",
            "then","now","look","only","come","its","over","think","also","back","after","use","two","how",
            "our","work","first","well","way","even","new","want","because","any","these","give","day",
            "most","us"});
    //https://en.wikipedia.org/wiki/Most_common_words_in_English
    QStringList Qwords = QStringList(words); //new QStringList with elements the same as words
    Qwords.removeDuplicates();
    for (int i = 0; i < words.size(); i++){
        if(HundredMostCommonWords.contains(words.at(i) ) ){
            Qwords.removeAt(i);
        }
    }

    // Qwords now contains words minus the common words
    int counter = 0;
    for (int i=previousLength; i < notes.size();i++){

        // If the word in notes is within the common words, continue the loop
        if (HundredMostCommonWords.contains(notes.at(i) ) ){
            continue;
        }
        // If the word in notes is within the "important" words (Qwords), ++ the counter
        if (Qwords.contains(notes.at(i) ) ){
            counter++;
        }
    }

    // Defining a arbitrary weightFactor to determine if there are enough relevant words
    int weightFactor = 4;
    if (counter*weightFactor > Qwords.size() ){
        return true;
    }

    else {
        qDebug() << "Relevant Word check failed";
        qDebug() << "# of relevant words in notes: " << counter;
        qDebug() << "# of relevant words " << Qwords.size();
        return false;
    }
 }

bool DeepReader::goodNotes() {
    // I should probably think of a way to read only the notes written
    // since the page was "turned"
    int minLength = 20;
    if (words.length() < minLength) {
        // there's very little text on the page. Go to the next page.
        return true;
    }
    // maybe have this as default, but figure out way for user to set this
    float weightFactor = 0.05;
    int minWordNum = words.length() * weightFactor;
    qDebug() << "words.length(): " << words.length();
    qDebug() << "minWordNum: " << minWordNum;
    QStringList notes = ui->texteditor->toPlainText().split(QRegExp("[,;.]*\\s+"));
    /*
     // Removing all previous notes
     for (i = 0; i < previousText; i++){
        notes.removeAt(0);
     }
     */
    qDebug() << notes;

    if ((notes.length() - previousText) > minWordNum && relevantNotes(notes, words, previousText) ){
        previousText = notes.length();
        qDebug() << "notes length: " << notes.length() << " notes from current page: " << notes.length() - previousText;
        return true;
    }

    return false;
}

void DeepReader::on_actionOpen_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
         tr("Open Document"), "", tr("PDF Files (*.pdf)"));
    if (filename.contains(".pdf")) { // could I do this in a less trivial way?
        doc = Poppler::Document::load(filename);
        if (!doc || doc->isLocked()) {
            qDebug() << "doc variable is null or locked";
            delete doc;
        }
        pageCounter = 0;
        qDebug() << (doc == NULL);
        showPage();
    }
}

void DeepReader::on_previous_clicked()
{
    if (doc != NULL) {
        --pageCounter;
        if (pageCounter < 0)
            pageCounter = 0;
        showPage();
    }
}

void DeepReader::on_next_clicked()
{
    // should study session end after last page?
    // i.e. endPage + 1, or after good notes have
    // been taken on that page
    if (doc != NULL) {
        if (studySession) {
            if (pageCounter >= endPage) {
                studySession == false;
                ++pageCounter;
                showPage();
            }
            else if (goodNotes()) {
                ++pageCounter;
                /*
                 previousText = QStringList notes = ui->texteditor->toPlainText().split(QRegExp("[,;.]*\\s+")).size();
                 */
                if (pageCounter >= doc->numPages())
                    pageCounter = doc->numPages() - 1;
                showPage();
            }
        } else {
            ++pageCounter;
            if (pageCounter >= doc->numPages())
                pageCounter = doc->numPages() - 1;
            showPage();
        }
    }
}

void DeepReader::on_lineEdit_returnPressed()
{
    if (doc != NULL) {
        QString text = ui->lineEdit->text();
        if(text.toInt() != NULL) {
            if (text.toInt() < doc->numPages()) {
                pageCounter = text.toInt();
                showPage();
            }
        } else {
            qDebug() << "string entered was not a number";
        }
        qDebug() << text;
    }
}

void DeepReader::on_actionSave_As_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), tr("Text files (*.txt)"));

    if (filename != "") {
        QFile file(filename);
        if (file.open(QIODevice::ReadWrite)) {
            QTextStream stream(&file);
            stream << ui->texteditor->toHtml();
            file.flush();
            file.close();
        }
        else {
            QMessageBox::critical(this, tr("Error"), tr("Could not save file"));
            return;
        }
    }
}

void DeepReader::on_actionOpen_Text_File_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
         tr("Open File"), "", tr("Text Files (*.txt *.rtf)"));
    QFile file(filename);
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream ReadFile(&file);
    if (filename.contains(".rtf")) {
        ui->texteditor->setHtml(ReadFile.readAll());
        previousText = ui->texteditor->toHtml().split(QRegExp("[\n]*\\s")).size();
    } else if (filename.contains(".txt")) {
        ui->texteditor->setPlainText(ReadFile.readAll());
        previousText = ui->texteditor->toPlainText().split(QRegExp("[\n]*\\s")).size();
    }
}

void DeepReader::on_actionUndo_triggered() {
    ui->texteditor->undo();
}

void DeepReader::on_actionRedo_triggered() {
    ui->texteditor->redo();
}

void DeepReader::on_actionCut_triggered() {
    ui->texteditor->cut();
}

void DeepReader::on_actionCopy_triggered() {
    ui->texteditor->copy();
}

void DeepReader::on_actionPaste_triggered() {
    ui->texteditor->paste();
}

void DeepReader::on_actionSelect_All_triggered() {
    ui->texteditor->selectAll();
}

void DeepReader::on_actionBullet_List_triggered() {
    ui->texteditor->insertHtml("<ul type='disc'><li></li></ul>");
}

void DeepReader::on_bold_clicked() {
    int bold = ui->texteditor->fontWeight();
    if(bold == 50) {
        ui->texteditor->setFontWeight(75);
    }
    else if(bold == 75) {
        ui->texteditor->setFontWeight(50);
    }
    else {
        QMessageBox::critical(this, tr("Error"), tr("Improper font weight"));
        return;
    }
}

void DeepReader::on_italic_clicked() {
    bool italic = ui->texteditor->fontItalic();
    ui->texteditor->setFontItalic(!italic);
}

void DeepReader::on_underline_clicked() {
    bool underline = ui->texteditor->fontUnderline();
    ui->texteditor->setFontUnderline(!underline);
}

void DeepReader::on_font_currentFontChanged() {
    QFont font = ui->font->currentFont();
    ui->texteditor->setCurrentFont(font);
}

void DeepReader::on_font_size_valueChanged() {
    int size = ui->font_size->value();
    ui->texteditor->setFontPointSize(size);
}

void DeepReader::on_alignleft_clicked() {
    ui->texteditor->setAlignment(Qt::AlignLeft);
}

void DeepReader::on_aligncenter_clicked() {
    ui->texteditor->setAlignment(Qt::AlignCenter);
}

void DeepReader::on_alignright_clicked() {
    ui->texteditor->setAlignment(Qt::AlignRight);
}

void DeepReader::on_alignjustify_clicked() {
    ui->texteditor->setAlignment(Qt::AlignJustify);
}

// text editor zoom-out
void DeepReader::on_zoom_out_clicked()
{
    ui->texteditor->zoomOut(3);
}

// text editor zoom-in
void DeepReader::on_zoom_in_clicked()
{
    ui->texteditor->zoomIn(3);
}

void DeepReader::on_start_clicked()
{
    // now that studySession is true, it should be impossible
    // to move to the next page without meeting certain requirements
    // I will have to add conditions in other functions based on
    // whether studySession is true or not
    if (doc != NULL) {
        studySession = true;
        startPage = ui->start_page->text().toInt();
        endPage = ui->end_page->text().toInt();
        if (pageCounter != startPage) {
            pageCounter = startPage;
            showPage();
        }
        // I may want to make words a private variable and pageText() a
        // private function so that I don't have to copy over an entire
        // list every time. This depends on what I end up having to extract
        // text for
        pageText();
        qDebug() << goodNotes();
    }
}

// pdf viewer zoom-out
void DeepReader::on_zoom_out_pdf_clicked()
{
    if (doc != NULL) {
        zoom -= 10;
        showPage();
    }
}

// pdf viewer zoom-in
void DeepReader::on_zoom_in_pdf_clicked()
{
    if (doc != NULL) {
        zoom += 10;
        showPage();
    }
}

// checks if time is up and displays message that time is up
void DeepReader::checkTime()
{
    double secondsPassed = (clock() - startTime) / CLOCKS_PER_SEC;
    if(secondsPassed >= timeDuration){
        //TO DO: display that time is up (need to figure out where we want to display time up)
    }
}

// find word
void DeepReader::on_search_returnPressed()
{
    // I need a way to go through pages
    // I could store current word as variable, and when enter is pressed again and it's
    // the same word, just go to the next page in the pages array
    // search args : const QString &text, SearchFlags flags = 0, Rotation rotate = 0
    if (!studySession) {
        QString word = ui->search->text();
        QList<int> pages; // pages on which the word is found
        for (int i = 0; i < doc->numPages(); ++i) {
            QList<QRectF> loc = doc->page(i)->search(word);
            if (!loc.empty()) {
                pages.append(i);
                qDebug() << i;
            }
        }
        if (!pages.empty()) {
            pageCounter = pages[0];
            showPage();
        }
    }
}
