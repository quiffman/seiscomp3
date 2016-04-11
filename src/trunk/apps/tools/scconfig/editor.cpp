/***************************************************************************
 *   Copyright (C) by GFZ Potsdam                                          *
 *                                                                         *
 *   You can redistribute and/or modify this program under the             *
 *   terms of the SeisComP Public License.                                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   SeisComP Public License for more details.                             *
 ***************************************************************************/

#include "editor.h"


namespace {


class LineNumberArea : public QWidget {
	public:
		LineNumberArea(ConfigEditor *editor) : QWidget(editor) {
			_editor = editor;
		}

		QSize sizeHint() const {
			return QSize(_editor->lineNumberAreaWidth(), 0);
		}

	protected:
		void paintEvent(QPaintEvent *event) {
			QAbstractTextDocumentLayout *layout = _editor->document()->documentLayout();
			int contentsY = _editor->verticalScrollBar()->value();
			qreal pageBottom = contentsY + _editor->viewport()->height();
			const QFontMetrics fm = fontMetrics();
			const int ascent = fontMetrics().ascent() + 1;
			int lineCount = 1;

			QPainter p(this);

			for ( QTextBlock block = _editor->document()->begin();
				block.isValid(); block = block.next(), ++lineCount ) {

				const QRectF boundingRect = layout->blockBoundingRect(block);

				QPointF position = boundingRect.topLeft();

				if ( position.y() + boundingRect.height() < contentsY )
					continue;

				if ( position.y() > pageBottom )
					break;

				const QString txt = QString::number(lineCount);
				p.drawText(width() - fm.width(txt) - 3, qRound(position.y() ) - contentsY + ascent, txt);
			}
		}

	private:
		ConfigEditor *_editor;
};


}



ConfigHighlighter::ConfigHighlighter(QTextDocument *parent)
: QSyntaxHighlighter(parent) {
	Rule rule;

	_keywordFormat.setForeground(QColor(128,128,0));
	QStringList keywordPatterns;
	keywordPatterns << "\\binclude\\b" << "\\bdel\\b"
	                << "\\btrue\\b" << "\\bfalse\\b";
	foreach (const QString &pattern, keywordPatterns) {
		rule.pattern = QRegExp(pattern);
		rule.format = _keywordFormat;
		_rules.append(rule);
	}

	_commentFormat.setForeground(Qt::darkGray);
	_commentFormat.setFontItalic(true);
	rule.pattern = QRegExp("#[^\n]*");
	rule.format = _commentFormat;
	_rules.append(rule);

	_quotationFormat.setForeground(Qt::darkGreen);
	rule.pattern = QRegExp("\".*\"");
	rule.format = _quotationFormat;
	_rules.append(rule);

	_variableFormat.setForeground(Qt::darkRed);
	rule.pattern = QRegExp("\\$\\{.*\\}");
	rule.format = _variableFormat;
	_rules.append(rule);

	QTextCharFormat whitespaceFormat;
	whitespaceFormat.setForeground(QColor(192,192,192));
	rule.pattern = QRegExp("\\s");
	rule.format = whitespaceFormat;
	_rules.append(rule);

	/*
	_placeHolderFormat.setForeground(Qt::darkMagenta);
	rule.pattern = QRegExp("@.*@");
	rule.format = _placeHolderFormat;
	_rules.append(rule);
	*/
}


void ConfigHighlighter::highlightBlock(const QString &text) {
	foreach ( const Rule &rule, _rules ) {
		QRegExp expression(rule.pattern);
		int index = expression.indexIn(text);
		while ( index >= 0 ) {
			int length = expression.matchedLength();
			setFormat(index, length, rule.format);
			index = expression.indexIn(text, index + length);
		}
	}
	setCurrentBlockState(0);
}


ConfigEditor::ConfigEditor(QWidget *parent) : QTextEdit(parent) {
	init();
}


ConfigEditor::ConfigEditor(const QString &text, QWidget *parent) : QTextEdit(text, parent) {
	init();
}


void ConfigEditor::init() {
	_errorLine = -1;

	QFont font("Monospace");
	font.setStyleHint(QFont::TypeWriter);
	setFont(font);

	new ConfigHighlighter(document());

	_lineNumbers = new LineNumberArea(this);
	QPalette pal = _lineNumbers->palette();
	pal.setColor(QPalette::WindowText, QColor(96,96,96));
	_lineNumbers->setPalette(pal);

#if QT_VERSION >= 0x040500
	QTextOption option = document()->defaultTextOption();
	option.setFlags(option.flags() | QTextOption::ShowTabsAndSpaces);
	document()->setDefaultTextOption(option);
#endif

	setTabStopWidth(fontMetrics().width(QLatin1Char('9'))*4);

	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateExtraSelections()));
	connect(document(), SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));

	connect(document()->documentLayout(), SIGNAL(update(const QRectF &)),
	        this, SLOT(layoutChanged(const QRectF &)));
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)),
	        this, SLOT(scrollBarValueChanged(int)));
}


void ConfigEditor::setErrorLine(int line) {
	_errorLine = line;
	updateExtraSelections();
}


void ConfigEditor::gotoLine(int line) {
	QTextCursor cur = textCursor();
	cur.movePosition(QTextCursor::Start);
	cur.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line-1);
	setTextCursor(cur);
	ensureCursorVisible();
}


void ConfigEditor::layoutChanged(const QRectF &) {
	updateLineNumberArea(rect(), 0);
}


void ConfigEditor::scrollBarValueChanged(int dy) {
	updateLineNumberArea(rect(), dy);
}


void ConfigEditor::updateExtraSelections() {
	QList<ExtraSelection> extraSelections;

	if ( !isReadOnly() ) {
		ExtraSelection selection;
		QColor lineColor = QColor(199,231,249);
		selection.format.setBackground(lineColor);
		selection.format.setProperty(QTextFormat::FullWidthSelection, true);
		selection.cursor = textCursor();
		selection.cursor.clearSelection();
		extraSelections.append(selection);
	}

	if ( _errorLine != -1 ) {
		ExtraSelection selection;
		QColor lineColor = QColor(255,128,128);
		selection.format.setBackground(lineColor);
		selection.format.setProperty(QTextFormat::FullWidthSelection, true);
		selection.cursor = textCursor();
		selection.cursor.movePosition(QTextCursor::Start);
		if ( selection.cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, _errorLine-1) ) {
			selection.cursor.clearSelection();
			extraSelections.append(selection);
		}
	}

	setExtraSelections(extraSelections);
}


int ConfigEditor::lineNumberAreaWidth() {
	int digits = 1;
	int max = qMax(1, document()->blockCount());
	while ( max >= 10 ) {
		max /= 10;
		++digits;
	}

	int space = 8 + fontMetrics().width(QLatin1Char('9')) * digits;

	return space;
}


void ConfigEditor::updateLineNumberAreaWidth(int /* newBlockCount */) {
	setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}


void ConfigEditor::updateLineNumberArea(const QRect &rect, int dy) {
	if ( rect.contains(viewport()->rect()) )
		updateLineNumberAreaWidth(0);

	update();
}


void ConfigEditor::resizeEvent(QResizeEvent *event) {
	QTextEdit::resizeEvent(event);

	QRect cr = contentsRect();
	_lineNumbers->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}



ConfigFileWidget::ConfigFileWidget(QWidget *parent) : QWidget(parent) {
	_editor = new ConfigEditor;
	//_editor->setFrameShape(QFrame::NoFrame);
	_errorlist = new QListWidget;
	//_errorlist->setFrameShape(QFrame::NoFrame);
	_errorlist->setAlternatingRowColors(true);

	QVBoxLayout *l = new QVBoxLayout;
	l->setMargin(0);
	l->setSpacing(0);
	QLabel *header = new QLabel;
	header->setText("Parsing issues");
	header->setMargin(2);

	l->addWidget(header);
	l->addWidget(_errorlist);

	QWidget *container = new QWidget;
	container->setLayout(l);

	_splitter = new QSplitter(Qt::Vertical);
	_splitter->addWidget(_editor);
	_splitter->addWidget(container);
	_splitter->setSizes(QList<int>() << 1000 << 1);

	l = new QVBoxLayout;
	//l->setMargin(0);
	l->addWidget(_splitter);
	setLayout(l);

	connect(_errorlist, SIGNAL(currentRowChanged(int)),
	        this, SLOT(setError(int)));
}


ConfigEditor *ConfigFileWidget::editor() const {
	return _editor;
}


bool ConfigFileWidget::loadFile(const QString &filename) {
	QFile file(filename);
	// File not found is actually not an error
	if ( !file.open(QIODevice::ReadOnly | QIODevice::Text) ) return false;

	QByteArray text = file.readAll();
	_editor->setPlainText(QString(text));

	return true;
}


bool ConfigFileWidget::saveFile(const QString &filename) {
	QFile file(filename);
	// File not found is actually not an error
	if ( !file.open(QIODevice::WriteOnly | QIODevice::Text) ) return false;

	QByteArray text;
	text += _editor->toPlainText();
	file.write(text);

	return true;
}


void ConfigFileWidget::setErrors(const QList<Error> &errors, bool selectFirst) {
	_errorlist->clear();

	for ( int i = 0; i < errors.size(); ++i ) {
		QListWidgetItem *line = new QListWidgetItem(QIcon(":/res/icons/error_small.png"),
		                                            QString("Line %1: %2")
			                                        .arg(errors[i].first)
			                                        .arg(errors[i].second));
		line->setData(Qt::UserRole, errors[i].first);
		_errorlist->addItem(line);
	}

	if ( _errorlist->count() > 0 ) {
		if ( selectFirst )
			_errorlist->setCurrentRow(0);
	}
	else
		_editor->setErrorLine(-1);
}


void ConfigFileWidget::setError(int r) {
	QListWidgetItem *item = _errorlist->item(r);
	if ( item == NULL ) return;

	int line = item->data(Qt::UserRole).toInt();
	_editor->setErrorLine(line);
	_editor->gotoLine(line);
	_editor->setFocus();
}
