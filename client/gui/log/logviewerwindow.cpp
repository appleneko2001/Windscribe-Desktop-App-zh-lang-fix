#include "logviewerwindow.h"

#include <QApplication>
#include <QFileDialog>
#include <QIcon>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>

#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "utils/mergelog.h"

namespace LogViewer {

LogViewerWindow::LogViewerWindow(QWidget *parent)
    : DPIScaleAwareWidget(parent), isColorHighlighting_(DEFAULT_COLOR_HIGHLIGHTING)
{
    setWindowFlag(Qt::Dialog);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowFlag(Qt::WindowMinimizeButtonHint, false);

    // todo change SVG to ICO file.
    setWindowIcon(ImageResourcesSvg::instance().getIndependentPixmap("BADGE_BLACK_ICON")->getScaledIcon());

    setWindowTitle("Windscribe Log");

    textEdit_ = new QPlainTextEdit(this);
    textEdit_->setReadOnly(true);

    cbMergePerLine_ = new QCheckBox(this);
    cbMergePerLine_->setText(tr("Merge all logs by timestamp"));
    cbMergePerLine_->setChecked(DEFAULT_MERGE_PER_LINE);
    connect(cbMergePerLine_, &QCheckBox::toggled, this, &LogViewerWindow::updateLog);

    cbWordWrap_ = new QCheckBox(this);
    cbWordWrap_->setText(tr("Word Wrap"));
    cbWordWrap_->setChecked(true);
    connect(cbWordWrap_, &QCheckBox::toggled, this, &LogViewerWindow::onWordWrapToggled);

    cbColorHighlighting_ = new QCheckBox(this);
    cbColorHighlighting_->setText(tr("Color highlighting"));
    cbColorHighlighting_->setChecked(isColorHighlighting_);
    connect(cbColorHighlighting_, &QCheckBox::toggled, this, &LogViewerWindow::updateColorHighlighting);

    btnExportLog_ = new QPushButton("this");
    btnExportLog_->setText(tr("Export to file..."));
    connect(btnExportLog_, SIGNAL(clicked(bool)), SLOT(onExportClick()));

    auto *hLayout = new QHBoxLayout();
    hLayout->setAlignment(Qt::AlignLeft);
    hLayout->addWidget(cbMergePerLine_);
    hLayout->addWidget(cbWordWrap_);
    hLayout->addWidget(cbColorHighlighting_);
    hLayout->addWidget(btnExportLog_);
    hLayout->addStretch(1);

    layout_ = new QVBoxLayout(this);
    layout_->setAlignment(Qt::AlignCenter);
    layout_->addLayout(hLayout);
    layout_->addWidget(textEdit_, 1);

    // make size of dialog to 70% of desktop size
    QRect desktopRc = screen()->availableGeometry();
    setGeometry(desktopRc.left() + desktopRc.width() * 0.3 / 2,
                desktopRc.top() + desktopRc.height() * 0.3 / 2,
                desktopRc.width() * 0.7,
                desktopRc.height() * 0.7);

    updateScaling();

    QTimer::singleShot(250, this, [this]() { updateLog(DEFAULT_MERGE_PER_LINE); });
}

LogViewerWindow::~LogViewerWindow()
{
}

void LogViewerWindow::updateLog(bool doMergePerLine)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    textEdit_->setPlainText(MergeLog::mergeLogs(doMergePerLine));
    highlightBlocks();
    QApplication::restoreOverrideCursor();
}

void LogViewerWindow::updateColorHighlighting(bool isColorHighlighting)
{
    isColorHighlighting_ = isColorHighlighting;
    highlightBlocks();
}

void LogViewerWindow::onExportClick()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save log"), QString(), tr("Text files (*.txt)"));
    if (!fileName.isEmpty())
    {
        QString log = MergeLog::mergePrevLogs(true);
        log += "================================================================================================================================================================================================\n";
        log += "================================================================================================================================================================================================\n";
        log += MergeLog::mergeLogs(true);

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(log.toLocal8Bit());
        }
        else
        {
            QMessageBox::information(this, tr("Export log"), tr("Failed to export log.  Make sure you have the correct permissions."));
        }
    }
}

void LogViewerWindow::updateScaling()
{
    textEdit_->setFont(*FontManager::instance().getFontWithCustomScale(currentScale(), 12, false));
}

void LogViewerWindow::highlightBlocks()
{
    const auto *doc = textEdit_->document();
    auto block = doc->begin();
    if (block == doc->end())
        return;

    const auto kGUIBlockBrush = QBrush(QColor(Qt::cyan).lighter(180));
    const auto kEngineBlockBrush = QBrush(QColor(Qt::yellow).lighter(180));
    const auto kServiceBlockBrush = QBrush(QColor(Qt::magenta).lighter(180));

    for (; block != doc->end(); block = block.next()) {
        auto blockFormat = block.blockFormat();
        if (block.text().isEmpty())
            continue;
        if (isColorHighlighting_) {
            switch (block.text()[0].toLatin1()) {
            case 'G':
                blockFormat.setBackground(kGUIBlockBrush);
                break;
            case 'E':
                blockFormat.setBackground(kEngineBlockBrush);
                break;
            case 'S':
                blockFormat.setBackground(kServiceBlockBrush);
                break;
            default:
                break;
            }
        } else {
            blockFormat.setBackground(QBrush());
        }
        QTextCursor(block).setBlockFormat(blockFormat);
    }
}

void LogViewerWindow::onWordWrapToggled(bool wordWrap)
{
    textEdit_->setWordWrapMode(wordWrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
}

} //namespace LogViewer
