/***************************************************************************
//
//    softProjector - an open source media projection software
//    Copyright (C) 2017  Vladislav Kobzar
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation version 3 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
***************************************************************************/

#include "biblewidget.hpp"
#include "ui_biblewidget.h"
#include "song.hpp"
//#include <time.h>

//double diffclock(clock_t clock1,clock_t clock2)
//{
//        double diffticks=clock1-clock2;
//        double diffms=(diffticks*1000)/CLOCKS_PER_SEC;
//        return diffms;
//}

BibleWidget::BibleWidget(QWidget *parent) :
    QWidget(parent), ui(new Ui::BibleWidget)
{
    ui->setupUi(this);
    on_hide_result_button_clicked();

    chapter_validator = new QIntValidator(1, 1, ui->chapter_ef);
    verse_validator = new QIntValidator(1, 1, ui->verse_ef);

    ui->chapter_ef->setValidator( chapter_validator );
    ui->verse_ef->setValidator( verse_validator );

    highlight = new HighlighterDelegate(ui->search_results_list);
    ui->search_results_list->setItemDelegate(highlight);
}

BibleWidget::~BibleWidget()
{
    delete chapter_validator;
    delete verse_validator;
    delete ui;
}

void BibleWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void BibleWidget::setSettings(BibleVersionSettings &sets)
{
    QString initial_bible = mySettings.operatorBible;
    mySettings = sets;
    loadBibles(initial_bible);
}

void BibleWidget::loadBibles(QString initialId)
{
    // if operator bible = "same", then set it to primary bible
    if(mySettings.operatorBible == "same")
        mySettings.operatorBible = mySettings.primaryBible;

    // make sure that program does not drop if no bible is present
    if(mySettings.operatorBible == "none")
        ui->btnLive->setEnabled(false);
    else
        ui->btnLive->setEnabled(true);

    // Get all active bible books
    if(mySettings.operatorBible != "none")
    {
        all_active_bible_books.clear();
        if (mySettings.operatorBible != mySettings.primaryBible &&
            mySettings.operatorBible != mySettings.secondaryBible &&
            mySettings.operatorBible != mySettings.trinaryBible)
        {
            bible.setBiblesId(mySettings.operatorBible);
            all_active_bible_books += bible.getFullBooks();
        }
        if (mySettings.trinaryBible != "none")
        {
            bible.setBiblesId(mySettings.trinaryBible);
            all_active_bible_books += bible.getFullBooks();
        }
        if (mySettings.secondaryBible != "none")
        {
            bible.setBiblesId(mySettings.secondaryBible);
            all_active_bible_books += bible.getFullBooks();
        }
        if (mySettings.primaryBible != "none")
        {
            bible.setBiblesId(mySettings.primaryBible);
            all_active_bible_books += bible.getFullBooks();
        }
    }

    // Check if primary bible is different that what has been loaded already
    // If it is different, then reload the bible list
    if(initialId!=mySettings.operatorBible)
    {
        bible.setBiblesId(mySettings.operatorBible);
        bible.loadOperatorBible();
        ui->listBook->clear();
        ui->listBook->addItems(bible.getBooks());
        ui->listBook->setCurrentRow(0);
    }
}

void BibleWidget::on_listBook_currentTextChanged(QString currentText)
{
    int s = ui->listBook->currentRow();
    if( s != -1 )
    {
        int max_chapter = bible.books.at(bible.getCurrentBookRow(currentText)).chapterCount;
        ui->listChapterNum->clear();
        for(int i=0; i<max_chapter; ++i)
            ui->listChapterNum->addItem(QString::number(i+1));
        chapter_validator->setTop(max_chapter);
        if( ui->listChapterNum->currentRow() != 0 )
            ui->listChapterNum->setCurrentRow(0);
    }
    else
    {
        // No bible book selected
        chapter_validator->setTop(1);
        ui->listChapterNum->clear();
    }
}

void BibleWidget::on_listChapterNum_currentTextChanged(QString currentText)
{
    int s = ui->listChapterNum->currentRow();
    if( s != -1 )
    {
        // This optimization is required in order for the bible filter entry field to work fast:
        if( currentBook != getCurrentBook() || currentChapter != currentText.toInt() )
        {
            currentBook = getCurrentBook();
            currentChapter = currentText.toInt();
            currentChapterList = bible.getChapter(bible.books.at(bible.getCurrentBookRow(currentBook)).bookId.toInt(), currentChapter);
        }

        ui->chapter_preview_list->clear();
        ui->chapter_preview_list->addItems(currentChapterList);
        ui->chapter_ef->setText(currentText);
        verse_validator->setTop(ui->chapter_preview_list->count());
        ui->chapter_preview_list->setCurrentRow(0);
    }
    else
    {
        ui->chapter_preview_list->clear();
    }
}

QString BibleWidget::getCurrentBook()
{
    return ui->listBook->currentItem()->text();
}

int BibleWidget::getCurrentChapter()
{
    return ui->listChapterNum->currentItem()->text().toInt();
}

/*
bool BibleWidget::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->spinVerse && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::KeyDown) {
            qDebug() << "DOWN KEY";
            // Special tab handling
            return true;
        } else
            return false;
    }
    return false;
}
*/

void BibleWidget::on_chapter_preview_list_currentRowChanged(int currentRow)
{
    ui->verse_ef->setText(QString::number(currentRow+1));
}

void BibleWidget::on_chapter_preview_list_doubleClicked(QModelIndex index)
{
    // Called when a chapter or verse is double clicked
    sendToProjector(true);
}

void BibleWidget::sendToProjector(bool add_to_history)
{
    bible.currentIdList = bible.previewIdList;
    QItemSelection selectedItems = ui->chapter_preview_list->selectionModel()->selection();
    if(selectedItems.count())
    {
        // Get the caption string to show above the show list (right-most list)
        QString cap = QString("%1 %2").arg(ui->listBook->currentItem()->text()).arg(ui->listChapterNum->currentItem()->text());
        emit goLive(bible.verseList, cap,selectedItems);
        if (add_to_history)
            addToHistory();
    }
}

void BibleWidget::on_lineEditBook_textChanged(QString text)
{
    // Called when the bible book filter field is modified.
    QList<BibleBook> operator_books = bible.getFullBooks();

    // Remove trailing spaces:
    text = text.trimmed();

    int chapter = 0;
    int verse = 0;

    // Check whether the user entered a search string that include book, chapter,
    // and verse. For example: "1King 3 13"
    QStringList search_words = text.split(" ");

    // Allows the user to use more than one space as a seperator:
    search_words.removeAll("");

    if( search_words.count() > 1 )
    {
        bool ok;
        int num1 = search_words.last().toInt(&ok);
        if( ok )
        {
            chapter = num1;
            search_words.removeLast();
            if( search_words.count() > 1 )
            {
                bool ok2;
                int num2 = search_words.last().toInt(&ok2);
                if( ok2 )
                {
                    search_words.removeLast();
                    chapter = num2;
                    verse = num1;
                }
            }
            text = search_words.join(" ");
        }
    }

    // Now search all books to find the matching book:
    if( text.isEmpty() )
    {
        // Show all bible books
        if( ui->listBook->count() != operator_books.count() )
        {
            // This is an important optimization
            ui->listBook->clear();
            ui->listBook->addItems(bible.getBooks());
        }
    }
    else
    {
        // Stores the filtered books of the operator bible
        QStringList filtered_books;
        // Stores which books from all active bibles have been filtered
        QStringList filtered_book_ids;

        bool has_num = text.at(0).isDigit();
        QString num_str = text.at(0);

        // If the search starts with a number remove the first two characters
        QString name_str = has_num ? text.remove(0, 1): text;
        // Allow for space inbetween num_str and name_str e.g. "1 Peter"
        name_str = name_str.trimmed();

        // Go through each book of every active bible
        for (int i = 0; i < all_active_bible_books.count(); i++)
        {
            BibleBook current_book = all_active_bible_books.at(i);

            // If the book is already in the filtered list, go to the next one
            if (!filtered_book_ids.contains(current_book.bookId))
            {
                QString current_book_name = current_book.book;

                // If number or search name don't match the
                // current book's title, go to the next one
                if( has_num && current_book_name.at(0) != num_str )
                    continue;
                if( !current_book_name.contains(name_str, Qt::CaseInsensitive) )
                    continue;

                // Go through each book in the operator bible
                for (int j = 0; j < operator_books.count(); j++)
                {
                    QString current_op_book_id = operator_books.at(j).bookId;

                    // If the current operator book is the same as the current book
                    if (current_op_book_id == current_book.bookId)
                    {
                        // Store that the book was added to the filtered list
                        filtered_book_ids.append(current_op_book_id);
                        // Add it to the filtered list in the operator bible's language
                        filtered_books.append(operator_books.at(j).book);
                        // Stop going through operator books since match was found
                        break;
                    }
                }
            }
        }

        if( ui->listBook->count() != filtered_books.count() )
        {
            // This is an important optimization
            // FIXME don't just check the count; check values
            ui->listBook->clear();
            ui->listBook->addItems(filtered_books);
        }
    }

    if( ui->listBook->count() > 0 )
        // Select the first row. This will take a longer time only if it will cause
        // a new chapter to be loaded into the preview
        ui->listBook->setCurrentRow(0);

    if( chapter != 0 && chapter <= ui->listChapterNum->count() )
    {
        if( ui->listChapterNum->currentRow() != (chapter-1) )
            ui->listChapterNum->setCurrentRow(chapter-1);
        if( verse != 0 && verse <= ui->chapter_preview_list->count() )
            ui->chapter_preview_list->setCurrentRow(verse-1);
    }
}

void BibleWidget::on_btnLive_clicked()
{
    sendToProjector(true);
}

void BibleWidget::on_verse_ef_textChanged(QString new_string)
{
    int value = new_string.toInt();
    ui->chapter_preview_list->setCurrentRow(value-1);
}

void BibleWidget::on_chapter_ef_textChanged(QString new_string)
{
    int value = new_string.toInt();
    ui->listChapterNum->setCurrentRow(value-1);
}

void BibleWidget::on_search_button_clicked()
{
    QString search_text = ui->search_ef->text();
    search_text = clean(search_text); // remove all none alphanumeric charecters

    // Make sure that there is some text to do a search for, if none, then return
    if(search_text.count()<1)
    {
        ui->search_ef->clear();
        ui->search_ef->setPlaceholderText(tr("Please enter search text"));
        return;
    }

    emit setWaitCursor();
    int type = ui->comboBoxSearchType->currentIndex();
    int range = ui->comboBoxSearchRange->currentIndex();

    QRegExp rx, rxh;
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    search_text.replace(" ","\\W*");
    if(type == 0)
    {
        // Search text phrase
        rx.setPattern(search_text);
        rxh.setPattern(search_text);
    }
    else if(type == 1)
    {
        // Search whole word exsact phrase only
        rx.setPattern("\\b"+search_text+"\\b");
        rxh.setPattern("\\b"+search_text+"\\b");
    }
    else if(type == 2)
    {
        // Search begining of every line
        rx.setPattern("^"+search_text);
        rxh.setPattern(search_text);
    }
    else if(type == 3 || type == 4)
    {
        // Search for any of the search words
        search_text.replace("\\W*","|");
        rx.setPattern("\\b("+search_text+")\\b");
        rxh.setPattern("\\b("+search_text+")\\b");
    }

    highlight->highlighter->setHighlightText(rxh.pattern()); // set highlighting rule

    if(range == 0) // Search entire Bible
        search_results = bible.searchBible((type == 4),rx);
    else if(range == 1) // Search current book only
        search_results = bible.searchBible((type == 4),rx,
                                           bible.books.at(bible.getCurrentBookRow(ui->listBook->currentItem()->text())).bookId.toInt());
    else if (range == 2) // Search current chapter only
        search_results = bible.searchBible((type == 4),rx,
                                           bible.books.at(bible.getCurrentBookRow(ui->listBook->currentItem()->text())).bookId.toInt(),
                                           ui->listChapterNum->currentItem()->text().toInt());

    ui->search_results_list->clear();

    if (!search_results.isEmpty()) // If have results, then show them
    {
        if( not ui->result_label->isVisible() )
        {
            ui->lineEditBook->clear();
            hidden_splitter_state = ui->results_splitter->saveState();
            ui->result_label->show();
            ui->result_count_label->show();
            ui->search_results_list->show();
            ui->hide_result_button->show();
            ui->search_layout->addItem(ui->results_layout);
            ui->results_splitter->restoreState(shown_splitter_state);
        }
        QStringList verse_list;
        int count = search_results.count();

        ui->result_count_label->setText(tr("Total\nresutls:\n%1").arg(count));

        for(int i(0);i<count;i++)
            verse_list.append(search_results.at(i).verse_text);
        ui->search_results_list->addItems(verse_list);
    }
    else // If no relust, notify the user and hide result list
        ui->result_count_label->setText(tr("No search\nresults."));

    emit setArrowCursor();
}

void BibleWidget::on_hide_result_button_clicked()
{
    shown_splitter_state = ui->results_splitter->saveState();
    ui->result_label->hide();
    ui->result_count_label->hide();
    ui->search_results_list->hide();
    ui->hide_result_button->hide();
    ui->search_layout->removeItem(ui->results_layout);
    ui->results_splitter->restoreState(hidden_splitter_state);
}

void BibleWidget::on_search_results_list_currentRowChanged(int currentRow)
{
    if (currentRow >=0)
    {
        QStringList all_books = bible.getBooks();

        if(ui->listBook->count() != all_books.count())
        {
            ui->listBook->clear();
            ui->listBook->addItems(all_books);
        }

        int row = all_books.indexOf(search_results.at(currentRow).book);
        ui->listBook->setCurrentRow(row);

        ui->chapter_ef->setText(search_results.at(currentRow).chapter);
        ui->verse_ef->setText(search_results.at(currentRow).verse);
    }
}

void BibleWidget::on_search_results_list_doubleClicked(QModelIndex index)
{
    on_search_results_list_currentRowChanged(index.row());
    on_btnLive_clicked();
}

void BibleWidget::addToHistory()
{
    BibleHistory b = getCurrentVerse();
    history_items.append(b);
    ui->history_listWidget->addItem(b.captionLong);
    // Scroll to latest history item
    ui->history_listWidget->scrollToBottom();
}

void BibleWidget::addToHistory(BibleHistory &b)
{
    history_items.append(b);
    ui->history_listWidget->addItem(b.captionLong);
}

void BibleWidget::clearHistory()
{
    ui->history_listWidget->clear();
    history_items.clear();
}

void BibleWidget::on_history_listWidget_currentRowChanged(int currentRow)
{
    if (currentRow >= 0)
    {
        BibleHistory bh = history_items.at(currentRow);
        setSelectedHistory(bh);
    }
}

void BibleWidget::setSelectedHistory(BibleHistory &b)
{
    QStringList all_books = bible.getBooks();
    if(ui->listBook->count()!=all_books.count())
    {
        ui->listBook->clear();
        ui->listBook->addItems(all_books);
    }
    QString bk;
    int ch,vr,vrl;
    bible.getVerseRef(b.verseIds,bk,ch,vr);
    vrl = bible.getVerseNumberLast(b.verseIds);

    ui->listBook->setCurrentRow(all_books.indexOf(bk));
    ui->chapter_ef->setText(QString::number(ch));
    QItemSelection sel;
    sel.select(ui->chapter_preview_list->model()->index(vr-1,0,QModelIndex()),
               ui->chapter_preview_list->model()->index(vrl-1,0,QModelIndex()));
    ui->chapter_preview_list->clearSelection();
    ui->verse_ef->setText(QString::number(vr));
    ui->chapter_preview_list->selectionModel()->select(sel,QItemSelectionModel::Select);
}

void BibleWidget::on_history_listWidget_doubleClicked(QModelIndex index)
{
    sendToProjector( false);
}

QByteArray BibleWidget::getHiddenSplitterState()
{
    if(ui->hide_result_button->isHidden())
        hidden_splitter_state = ui->results_splitter->saveState();
    return hidden_splitter_state;
}

QByteArray BibleWidget::getShownSplitterState()
{
    if(!ui->hide_result_button->isHidden())
        shown_splitter_state = ui->results_splitter->saveState();
    return shown_splitter_state;
}

void BibleWidget::setHiddenSplitterState(QByteArray& state)
{
    hidden_splitter_state = state;
    ui->results_splitter->restoreState(hidden_splitter_state);
}

void BibleWidget::setShownSplitterState(QByteArray& state)
{
    shown_splitter_state = state;
}

BibleHistory BibleWidget::getCurrentVerse()
{
    BibleHistory b;
    QString selected_ids;

    QString book = ui->listBook->currentItem()->text();
    QString chapter = ui->chapter_ef->text();

    int first_selected(-1),last_selected(-1);
    for(int i(0);i<ui->chapter_preview_list->count();++i)
    {
        if(ui->chapter_preview_list->item(i)->isSelected())
        {
            if(first_selected == -1)
                first_selected = i;
            last_selected = i;
            selected_ids += bible.previewIdList.at(i) + ",";
        }
    }
    selected_ids.chop(1);

    QString verse_text = ui->chapter_preview_list->item(first_selected)->text().trimmed();
    b.verseIds = selected_ids;

    if(first_selected==last_selected)
    {
        b.caption = book + " " + chapter + ":" + QString::number(first_selected+1);
        b.captionLong = book + " " + chapter + ":" + verse_text;
    }
    else
    {   // Create multi verse caption for display
        int f(first_selected+1), l(last_selected+1),j(0);
        QString v=verse_text,p=".";
        while(v.at(j)!=p.at(0))
            ++j;
        v = v.remove(0,j);

        b.caption = book + " " + chapter + ":" + QString::number(f) + "-" + QString::number(l);
        b.captionLong = book + " " + chapter + ":" + QString::number(f) + "-" + QString::number(l) + v + "...";
    }

    return b;
}

bool BibleWidget::isVerseSelected()
{
    if(ui->chapter_preview_list->selectedItems().count() >= 1)
        return true;
    else
        return false;
}

void BibleWidget::setBibleBookActive()
{
    ui->lineEditBook->setFocus();
    ui->lineEditBook->selectAll();
}

void BibleWidget::setBibleSearchActive()
{
    ui->search_ef->setFocus();
    ui->search_ef->selectAll();
}

bool BibleWidget::nextBook()
{
    int current_book_row = ui->listBook->currentRow();
    if (current_book_row < (ui->listBook->count() - 1))
    {
        ui->listBook->setCurrentRow(current_book_row + 1);
        return true;
    }
    return false;
}

bool BibleWidget::previousBook()
{
     int current_book_row = ui->listBook->currentRow();
     if (current_book_row > 0)
     {
         ui->listBook->setCurrentRow(current_book_row - 1);
         return true;
     }
     return false;
}

void BibleWidget::nextChapter()
{
    int current_chapter_row = ui->listChapterNum->currentRow();
    if (current_chapter_row < (ui->listChapterNum->count() - 1))
    {
        ui->listChapterNum->setCurrentRow(current_chapter_row + 1);
        sendToProjector(false);
    }
    else
    {
        if (nextBook())
            sendToProjector(false);
    }
}

void BibleWidget::previousChapter()
{
    int current_chapter_row = ui->listChapterNum->currentRow();
    if (current_chapter_row > 0)
    {
        ui->listChapterNum->setCurrentRow(current_chapter_row - 1);
        ui->chapter_preview_list->setCurrentRow(ui->chapter_preview_list->count() - 1);
        sendToProjector(false);
    }
    else
    {
        if (previousBook())
        {
            ui->listChapterNum->setCurrentRow(ui->listChapterNum->count() - 1);
            ui->chapter_preview_list->setCurrentRow(ui->chapter_preview_list->count() - 1);
            sendToProjector(false);
        }
    }
}
