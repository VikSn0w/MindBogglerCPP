#include "MainWindow.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtGui/QKeySequence>
#include <QtGui/QPixmap>
#include <QtCore/QStandardPaths>
#include <QtGui/QTextDocument>
#include <sstream>

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent), currentColor(255, 255, 0, 90), breakpointColor(255, 0, 0, 90) {
    setWordWrapMode(QTextOption::NoWrap);

    auto* shortcutBreakpoint = new QAction(this);
    shortcutBreakpoint->setShortcut(QKeySequence("F9"));
    connect(shortcutBreakpoint, &QAction::triggered, this, &CodeEditor::toggleBreakpointAtCaret);
    addAction(shortcutBreakpoint);
}

void CodeEditor::toggleBreakpointAtCaret() {
    int idx = textCursor().position();
   
    idx = std::max(0, std::min(idx, static_cast<int>(toPlainText().length() - 1)));

    if (breakpointIndices.count(idx)) {
        breakpointIndices.erase(idx);
    } else {
        breakpointIndices.insert(idx);
    }

    updateHighlighting(-1);
}

void CodeEditor::updateHighlighting(int currentPc) {
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextDocument* doc = document();

    auto makeSel = [this](int start, int length, const QColor& color) {
        QTextEdit::ExtraSelection sel;
        QTextCharFormat fmt;
        fmt.setBackground(color);
        sel.format = fmt;

        QTextCursor cur = textCursor();
        cur.setPosition(std::max(0, std::min(start, document()->characterCount() - 1)));
        cur.setPosition(std::max(0, std::min(start + std::max(1, length), document()->characterCount() - 1)),
                       QTextCursor::KeepAnchor);
        sel.cursor = cur;
        return sel;
    };

    if (currentPc >= 0 && currentPc < doc->characterCount()) {
        extraSelections.append(makeSel(currentPc, 1, currentColor));
    }

    for (int i : breakpointIndices) {
        if (i >= 0 && i < doc->characterCount()) {
            extraSelections.append(makeSel(i, 1, breakpointColor));
        }
    }

    setExtraSelections(extraSelections);
}

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Settings");
    setModal(true);
    resize(450, 400);

    auto* layout = new QVBoxLayout(this);

   
    auto* pointerGroup = new QGroupBox("Pointer Behavior");
    auto* pointerLayout = new QVBoxLayout(pointerGroup);

    pointerBehaviorGroup = new QButtonGroup(this);

    clampRadio = new QRadioButton("Clamp (safe - stays at boundaries)");
    wrapRadio = new QRadioButton("Wrap-around (circular memory)");
    errorRadio = new QRadioButton("Error on overflow/underflow");

    pointerBehaviorGroup->addButton(clampRadio, static_cast<int>(PointerBehavior::CLAMP));
    pointerBehaviorGroup->addButton(wrapRadio, static_cast<int>(PointerBehavior::WRAP));
    pointerBehaviorGroup->addButton(errorRadio, static_cast<int>(PointerBehavior::ERROR));

    clampRadio->setChecked(true);

    pointerLayout->addWidget(clampRadio);
    pointerLayout->addWidget(wrapRadio);
    pointerLayout->addWidget(errorRadio);

    auto* descLabel = new QLabel(
        "• Clamp: Pointer stops at memory boundaries (0 and memory_size-1)\n"
        "• Wrap-around: Pointer wraps to opposite end when crossing boundaries\n"
        "• Error: Throw exception when pointer goes out of bounds"
    );
    descLabel->setStyleSheet("color: gray; font-size: 9pt;");
    pointerLayout->addWidget(descLabel);

    layout->addWidget(pointerGroup);

   
    auto* cellGroup = new QGroupBox("Cell Value Behavior");
    auto* cellLayout = new QVBoxLayout(cellGroup);

    cellBehaviorGroup = new QButtonGroup(this);

    cellWrapRadio = new QRadioButton("Wrap (0-255, standard Brainfuck)");
    cellUnlimitedRadio = new QRadioButton("Unlimited (allow values beyond 0-255)");
    cellErrorRadio = new QRadioButton("Error on underflow/overflow");

    cellBehaviorGroup->addButton(cellWrapRadio, static_cast<int>(CellBehavior::WRAP));
    cellBehaviorGroup->addButton(cellUnlimitedRadio, static_cast<int>(CellBehavior::UNLIMITED));
    cellBehaviorGroup->addButton(cellErrorRadio, static_cast<int>(CellBehavior::ERROR));

    cellWrapRadio->setChecked(true);

    cellLayout->addWidget(cellWrapRadio);
    cellLayout->addWidget(cellUnlimitedRadio);
    cellLayout->addWidget(cellErrorRadio);

    auto* cellDesc = new QLabel(
        "• Wrap: Cell values wrap around 0-255 (255+1=0, 0-1=255)\n"
        "• Unlimited: Cell values can exceed 0-255 range (useful for calculations)\n"
        "• Error: Throw exception when cell goes below 0 or above 255"
    );
    cellDesc->setStyleSheet("color: gray; font-size: 9pt;");
    cellLayout->addWidget(cellDesc);

    layout->addWidget(cellGroup);

   
    auto* buttonLayout = new QHBoxLayout();
    okButton = new QPushButton("OK");
    cancelButton = new QPushButton("Cancel");

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

PointerBehavior SettingsDialog::getPointerBehavior() const {
    return static_cast<PointerBehavior>(pointerBehaviorGroup->checkedId());
}

void SettingsDialog::setPointerBehavior(PointerBehavior behavior) {
    switch (behavior) {
        case PointerBehavior::CLAMP: clampRadio->setChecked(true); break;
        case PointerBehavior::WRAP: wrapRadio->setChecked(true); break;
        case PointerBehavior::ERROR: errorRadio->setChecked(true); break;
    }
}

CellBehavior SettingsDialog::getCellBehavior() const {
    return static_cast<CellBehavior>(cellBehaviorGroup->checkedId());
}

void SettingsDialog::setCellBehavior(CellBehavior behavior) {
    switch (behavior) {
        case CellBehavior::WRAP: cellWrapRadio->setChecked(true); break;
        case CellBehavior::UNLIMITED: cellUnlimitedRadio->setChecked(true); break;
        case CellBehavior::ERROR: cellErrorRadio->setChecked(true); break;
    }
}

CompileOutputDialog::CompileOutputDialog(QWidget* parent, const QString& title, const QString& content)
    : QDialog(parent) {
    setWindowTitle(title);
    resize(800, 600);

    auto* layout = new QVBoxLayout(this);

    textArea = new QPlainTextEdit();
    textArea->setPlainText(content);
    textArea->setReadOnly(true);

    layout->addWidget(textArea);

    auto* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn);
}

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("About Mind Boggler");
    setModal(true);
    setFixedSize(500, 400);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(30, 30, 30, 30);

   
    auto* headerLayout = new QHBoxLayout();

    auto* iconLabel = new QLabel();
    iconLabel->setFixedSize(64, 64);
    iconLabel->setScaledContents(true);

    QPixmap pixmap("resources/icon.png");
    if (pixmap.isNull()) {
        QPixmap fallbackPixmap(64, 64);
        fallbackPixmap.fill(QColor(100, 149, 237));
        iconLabel->setPixmap(fallbackPixmap);
    } else {
        iconLabel->setPixmap(pixmap);
    }

    auto* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(5);

    auto* programName = new QLabel("MindBoggler++");
    programName->setStyleSheet("font-size: 24px; font-weight: bold; color: #2c3e50;");

    auto* subtitle = new QLabel("Brainfuck C++ IDE");
    subtitle->setStyleSheet("font-size: 14px; color: #7f8c8d; font-style: italic;");

    auto* versionLabel = new QLabel("Version 1.0.0");
    versionLabel->setStyleSheet("font-size: 12px; color: #95a5a6;");

    titleLayout->addWidget(programName);
    titleLayout->addWidget(subtitle);
    titleLayout->addWidget(versionLabel);
    titleLayout->addStretch();

    headerLayout->addWidget(iconLabel);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    layout->addLayout(headerLayout);

   
    auto* description = new QLabel(
        "A comprehensive Integrated Development Environment for the Brainfuck programming language. "
        "Features include syntax highlighting, debugging capabilities, memory visualization, "
        "configurable interpreter behaviors, and code analysis tools. Now ported to C++ with Qt6!"
    );
    description->setWordWrap(true);
    description->setStyleSheet("font-size: 12px; color: #34495e; line-height: 1.4;");
    description->setAlignment(Qt::AlignJustify);

    layout->addWidget(description);

   
    auto* separator = new QLabel();
    separator->setFixedHeight(1);
    separator->setStyleSheet("background-color: #bdc3c7; margin: 10px 0;");
    layout->addWidget(separator);

   
    auto* linksLayout = new QVBoxLayout();
    linksLayout->setSpacing(10);

    auto* linksTitle = new QLabel("Links");
    linksTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50;");
    linksLayout->addWidget(linksTitle);

   
    auto* githubLayout = new QHBoxLayout();
    auto* githubIcon = new QLabel("🚀");
    githubIcon->setStyleSheet("font-size: 16px;");

    auto* githubLink = new QPushButton("View on GitHub");
    githubLink->setStyleSheet(
        "QPushButton { background: none; border: none; color: #3498db; "
        "text-decoration: underline; font-size: 12px; text-align: left; padding: 2px; } "
        "QPushButton:hover { color: #2980b9; }"
    );
    githubLink->setCursor(Qt::PointingHandCursor);
    connect(githubLink, &QPushButton::clicked, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/VikSn0w/MindBoggler"));
    });

    githubLayout->addWidget(githubIcon);
    githubLayout->addWidget(githubLink);
    githubLayout->addStretch();

    linksLayout->addLayout(githubLayout);

    auto* linkedinLayout = new QHBoxLayout();
    auto* linkedinIcon = new QLabel("💼");
    linkedinIcon->setStyleSheet("font-size: 16px;");

    auto* linkedinLink = new QPushButton("Find me on LinkedIn");
    linkedinLink->setStyleSheet(
        "QPushButton { background: none; border: none; color: #3498db; "
        "text-decoration: underline; font-size: 12px; text-align: left; padding: 2px; } "
        "QPushButton:hover { color: #2980b9; }"
    );
    linkedinLink->setCursor(Qt::PointingHandCursor);
    connect(githubLink, &QPushButton::clicked, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/VikSn0w/MindBogglerCPP"));
    });

    linkedinLayout->addWidget(linkedinIcon);
    linkedinLayout->addWidget(linkedinLink);
    linkedinLayout->addStretch();

    linksLayout->addLayout(linkedinLayout);
    layout->addLayout(linksLayout);

    layout->addStretch();

   
    auto* copyrightLabel = new QLabel("2025 C++ Port - Under GPL-3.0 license");
    copyrightLabel->setStyleSheet("font-size: 10px; color: #95a5a6;");
    copyrightLabel->setAlignment(Qt::AlignCenter);

    auto* creditsLabel = new QLabel("Built with C++ and Qt6");
    creditsLabel->setStyleSheet("font-size: 10px; color: #95a5a6;");
    creditsLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(copyrightLabel);
    layout->addWidget(creditsLabel);

   
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* closeButton = new QPushButton("Close");
    closeButton->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; "
        "padding: 8px 20px; border-radius: 4px; font-size: 12px; } "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #21618c; }"
    );
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    closeButton->setDefault(true);

    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      interp(std::make_unique<Interpreter>()),
      executionMode(2),
      pausedAtBreakpoint(false) {

    setWindowTitle("Mind Boggler - Brainfuck C++ IDE");
    resize(1200, 800);
    setWindowIcon(QIcon("icon.png"));

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::onTimer);

    timerIntervals = {{0, 100}, {1, 500}, {2, 1}};

    interp->configure(settings.pointerBehavior, settings.cellBehavior);
    interp->setInputCallback([this]() { return requestInput(); });

    buildUI();
    connectActions();
    loadSample();
}

void MainWindow::buildUI() {
    editor = new CodeEditor();
    output = new QPlainTextEdit();
    output->setReadOnly(true);
    output->setPlaceholderText("Program output will appear here…");

    memTable = new QTableWidget(32, 16);
    QStringList headers;
    for (int i = 0; i < 16; ++i) {
        headers << QString("%1").arg(i, 0, 16).toUpper();
    }
    memTable->setHorizontalHeaderLabels(headers);

    QStringList rowHeaders;
    for (int i = 0; i < 32; ++i) {
        rowHeaders << QString("%1").arg(i * 16, 4, 16, QChar('0')).toUpper();
    }
    memTable->setVerticalHeaderLabels(rowHeaders);

    memTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memTable->setSelectionMode(QAbstractItemView::NoSelection);

   
    btnRun = new QPushButton("Run");
    btnStep = new QPushButton("Step");
    btnPause = new QPushButton("Pause");
    btnResume = new QPushButton("Resume");
    btnReset = new QPushButton("Reset");
    btnClearOut = new QPushButton("Clear Output");

    btnResume->hide();

   
    modeGroup = new QButtonGroup(this);
    modeDebug = new QRadioButton("Debug");
    modeSlow = new QRadioButton("Slow (2/sec)");
    modeFast = new QRadioButton("Fast");
    modeFast->setChecked(true);

    modeGroup->addButton(modeDebug, 0);
    modeGroup->addButton(modeSlow, 1);
    modeGroup->addButton(modeFast, 2);

    connect(modeGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::onModeChanged);

   
    auto* modeLayout = new QHBoxLayout();
    modeLayout->addWidget(new QLabel("Mode:"));
    modeLayout->addWidget(modeDebug);
    modeLayout->addWidget(modeSlow);
    modeLayout->addWidget(modeFast);
    modeLayout->addStretch();

    auto* controls = new QHBoxLayout();
    controls->addWidget(btnRun);
    controls->addWidget(btnStep);
    controls->addWidget(btnPause);
    controls->addWidget(btnResume);
    controls->addWidget(btnReset);
    controls->addWidget(btnClearOut);
    controls->addStretch(1);

    auto* left = new QWidget();
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->addWidget(new QLabel("Code"));
    leftLayout->addWidget(editor);
    leftLayout->addLayout(modeLayout);
    leftLayout->addLayout(controls);

    auto* right = new QWidget();
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->addWidget(new QLabel("Output"));
    rightLayout->addWidget(output);
    rightLayout->addWidget(new QLabel("Memory (hex grid around pointer)"));
    rightLayout->addWidget(memTable);

    auto* splitter = new QSplitter();
    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    setCentralWidget(splitter);

   
    auto* tb = addToolBar("Main");
    actOpen = new QAction("Open…", this);
    actSave = new QAction("Save As…", this);
    actCheck = new QAction("Check Syntax", this);
    actBreak = new QAction("Toggle Breakpoint (F9)", this);
    actBreak->setShortcut(QKeySequence("F9"));
    actCompile = new QAction("Compile & Show", this);
    actPseudocode = new QAction("Generate Pseudocode", this);
    actSettings = new QAction("Settings…", this);
    actAbout = new QAction("About…", this);

    tb->addAction(actOpen);
    tb->addAction(actSave);
    tb->addSeparator();
    tb->addAction(actCheck);
    tb->addAction(actCompile);
    tb->addAction(actPseudocode);
    tb->addAction(actBreak);
    tb->addSeparator();
    tb->addAction(actSettings);
    tb->addAction(actAbout);

    status = statusBar();
    updateStatus();
    refreshMemory();
}

void MainWindow::connectActions() {
    connect(btnRun, &QPushButton::clicked, this, &MainWindow::onRun);
    connect(btnStep, &QPushButton::clicked, this, &MainWindow::onStep);
    connect(btnPause, &QPushButton::clicked, this, &MainWindow::onPause);
    connect(btnResume, &QPushButton::clicked, this, &MainWindow::onResume);
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::onReset);
    connect(btnClearOut, &QPushButton::clicked, [this]() { output->setPlainText(""); });

    connect(actOpen, &QAction::triggered, this, &MainWindow::onOpen);
    connect(actSave, &QAction::triggered, this, &MainWindow::onSave);
    connect(actCheck, &QAction::triggered, this, &MainWindow::onCheck);
    connect(actBreak, &QAction::triggered, editor, &CodeEditor::toggleBreakpointAtCaret);
    connect(actCompile, &QAction::triggered, this, &MainWindow::onCompile);
    connect(actPseudocode, &QAction::triggered, this, &MainWindow::onPseudocode);
    connect(actSettings, &QAction::triggered, this, &MainWindow::onSettings);
    connect(actAbout, &QAction::triggered, this, &MainWindow::onAbout);
}

std::string MainWindow::requestInput() {
    bool ok;
    QString input = QInputDialog::getText(this, "Input Required",
                                         "Enter input for ',' command:",
                                         QLineEdit::Normal, "", &ok);
    if (ok) {
        return input.toStdString();
    }
    return "";
}

void MainWindow::onModeChanged() {
    auto* button = qobject_cast<QAbstractButton*>(sender());
    if (!button) return;

    executionMode = modeGroup->id(button);

    QStringList modeNames = {"Debug", "Slow (2/sec)", "Fast"};
    status->showMessage(modeNames[executionMode] + " mode enabled", 2000);

    if (timer->isActive()) {
        timer->setInterval(timerIntervals[executionMode]);
    }
}

void MainWindow::updateButtonStates() {
    bool running = timer->isActive();
    bool paused = pausedAtBreakpoint;

    btnRun->setEnabled(!running && !paused);
    btnStep->setEnabled(!running);
    btnPause->setEnabled(running && !paused);
    btnResume->setVisible(paused);
    btnReset->setEnabled(true);
}

void MainWindow::onRun() {
    if (!interp->isRunning()) {
        loadInterpreterFromUI();
    }

    pausedAtBreakpoint = false;
    timer->setInterval(timerIntervals[executionMode]);
    timer->start();
    updateButtonStates();
}

void MainWindow::onStep() {
    if (!interp->isRunning() && interp->getPc() == 0) {
        loadInterpreterFromUI();
    }

    timer->stop();
    pausedAtBreakpoint = false;

    executeDebugStep();
    updateButtonStates();
}

void MainWindow::onPause() {
    timer->stop();
    pausedAtBreakpoint = false;
    updateButtonStates();
}

void MainWindow::onResume() {
    pausedAtBreakpoint = false;
    timer->setInterval(timerIntervals[executionMode]);
    timer->start();
    updateButtonStates();
}

void MainWindow::onReset() {
    timer->stop();
    pausedAtBreakpoint = false;
    interp->reset();
    interp->configure(settings.pointerBehavior, settings.cellBehavior);
    output->setPlainText("");
    updateStatus();
    refreshMemory();
    editor->updateHighlighting(-1);
    updateButtonStates();
}

void MainWindow::onOpen() {
    QString path = QFileDialog::getOpenFileName(this, "Open Brainfuck file", "",
                                               "Brainfuck (*.bf *.b);;All Files (*)");
    if (!path.isEmpty()) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            editor->setPlainText(in.readAll());
        }
    }
}

void MainWindow::onSave() {
    QString path = QFileDialog::getSaveFileName(this, "Save Brainfuck file", "program.bf",
                                               "Brainfuck (*.bf *.b);;All Files (*)");
    if (!path.isEmpty()) {
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << editor->toPlainText();
        }
    }
}

void MainWindow::onCheck() {
    std::string program = editor->toPlainText().toStdString();
    Interpreter tempInterp;
    tempInterp.loadProgram(program, "");
    auto errors = tempInterp.checkProgramSyntax();

    if (errors.empty()) {
        QMessageBox::information(this, "Syntax", "No syntax errors detected.");
    } else {
        QString msg;
        for (size_t i = 0; i < std::min(errors.size(), size_t(200)); ++i) {
            const auto& [pos, ch] = errors[i];
            msg += QString("pos %1: '%2'\n").arg(pos).arg(ch);
        }
        QMessageBox::warning(this, "Syntax",
                           QString("Found %1 issue(s):\n%2").arg(errors.size()).arg(msg));
    }
}

void MainWindow::onSettings() {
    SettingsDialog dialog(this);
    dialog.setPointerBehavior(settings.pointerBehavior);
    dialog.setCellBehavior(settings.cellBehavior);

    if (dialog.exec() == QDialog::Accepted) {
        settings.pointerBehavior = dialog.getPointerBehavior();
        settings.cellBehavior = dialog.getCellBehavior();

        interp->configure(settings.pointerBehavior, settings.cellBehavior);

        QMap<PointerBehavior, QString> pointerNames = {
            {PointerBehavior::CLAMP, "Clamp"},
            {PointerBehavior::WRAP, "Wrap-around"},
            {PointerBehavior::ERROR, "Error on overflow"}
        };
        QMap<CellBehavior, QString> cellNames = {
            {CellBehavior::WRAP, "Wrap (0-255)"},
            {CellBehavior::UNLIMITED, "Unlimited"},
            {CellBehavior::ERROR, "Error on overflow"}
        };

        QString pointerName = pointerNames[settings.pointerBehavior];
        QString cellName = cellNames[settings.cellBehavior];

        status->showMessage(QString("Settings updated: Pointer=%1, Cells=%2")
                          .arg(pointerName).arg(cellName), 3000);
    }
}

void MainWindow::onCompile() {
    std::string program = editor->toPlainText().toStdString();
    Interpreter tempInterp;
    tempInterp.loadProgram(program, "");

    try {
        auto compiled = tempInterp.compileProgram();

        int originalOps = 0;
        for (char c : program) {
            if (std::string("[].,<>+-").find(c) != std::string::npos) {
                originalOps++;
            }
        }

        int compiledOps = static_cast<int>(compiled.size());

       
        int optimizations = 0;
        for (const auto& [cmd, arg] : compiled) {
            if ((cmd == '+' || cmd == '-' || cmd == '<' || cmd == '>') && arg > 1) {
                optimizations += arg - 1;
            }
        }

        double efficiency = originalOps > 0 ? (optimizations * 100.0 / originalOps) : 0.0;

        QString info;
        info += QString("Original operations: %1\n").arg(originalOps);
        info += QString("Compiled operations: %1\n").arg(compiledOps);
        info += QString("Operations saved by optimization: %1\n").arg(optimizations);
        info += QString("Efficiency improvement: %1%\n\n").arg(efficiency, 0, 'f', 1);
        info += "Compiled instructions:\n";
        info += QString("-").repeated(40) + "\n";

        for (size_t i = 0; i < compiled.size(); ++i) {
            const auto& [cmd, arg] = compiled[i];
            if (cmd == '.' || cmd == ',') {
                info += QString("%1: %2\n").arg(i, 3).arg(cmd);
            } else {
                info += QString("%1: %2 %3\n").arg(i, 3).arg(cmd).arg(arg);
            }
        }

        CompileOutputDialog dialog(this, "Compiled Program Analysis", info);
        dialog.exec();

    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Compilation Error",
                           QString("Error compiling program: %1").arg(e.what()));
    }
}

void MainWindow::onPseudocode() {
    std::string program = editor->toPlainText().toStdString();
    Interpreter tempInterp;
    tempInterp.configure(settings.pointerBehavior, settings.cellBehavior);
    tempInterp.loadProgram(program, "");

    try {
        std::string pseudocode = tempInterp.generatePseudocode();
        CompileOutputDialog dialog(this, "Generated Pseudocode", QString::fromStdString(pseudocode));
        dialog.exec();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Pseudocode Error",
                           QString("Error generating pseudocode: %1").arg(e.what()));
    }
}

void MainWindow::onAbout() {
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::loadInterpreterFromUI() {
    std::string program = editor->toPlainText().toStdString();
    interp->reset();
    interp->loadProgram(program, "");
    interp->configure(settings.pointerBehavior, settings.cellBehavior);
    interp->setInputCallback([this]() { return requestInput(); });
    output->setPlainText("");

    bool shouldHighlight = (executionMode == 0) || (!timer->isActive());
    editor->updateHighlighting(shouldHighlight ? interp->getPc() : -1);
}

bool MainWindow::executeOneStep() {
    if (executionMode == 2) {
        return executeFastChunk();
    } else {
        return executeDebugStep();
    }
}

bool MainWindow::executeFastChunk() {
    try {
       
        if (editor->getBreakpoints().count(interp->getPc())) {
            timer->stop();
            pausedAtBreakpoint = true;
            updateButtonStates();
            updateUIAfterStep();
            status->showMessage("Paused at breakpoint", 3000);
            return false;
        }

        bool moreNeeded = interp->runProgramFastInterruptible(50000);

        if (!moreNeeded) {
            timer->stop();
            updateButtonStates();
        }

        updateUIAfterStep();
        return moreNeeded;

    } catch (const PointerOverflowError& e) {
        timer->stop();
        pausedAtBreakpoint = false;
        updateButtonStates();
        QMessageBox::critical(this, "Pointer Overflow", QString("Pointer overflow error: %1").arg(e.what()));
        return false;
    } catch (const CellOverflowError& e) {
        timer->stop();
        pausedAtBreakpoint = false;
        updateButtonStates();
        QMessageBox::critical(this, "Cell Overflow", QString("Cell overflow error: %1").arg(e.what()));
        return false;
    } catch (const std::exception& e) {
        timer->stop();
        pausedAtBreakpoint = false;
        updateButtonStates();
        QMessageBox::critical(this, "Runtime Error", QString("Execution error: %1").arg(e.what()));
        return false;
    }
}

bool MainWindow::executeDebugStep() {
    if (timer->isActive() &&
        editor->getBreakpoints().count(interp->getPc()) &&
        !pausedAtBreakpoint) {
        timer->stop();
        pausedAtBreakpoint = true;
        updateButtonStates();
        updateUIAfterStep();
        status->showMessage("Paused at breakpoint", 3000);
        return false;
    }

    try {
        bool advanced = interp->step();
        updateUIAfterStep();

        if (!advanced) {
            timer->stop();
            pausedAtBreakpoint = false;
            updateButtonStates();
        }

        return advanced;

    } catch (const PointerOverflowError& e) {
        timer->stop();
        pausedAtBreakpoint = false;
        updateButtonStates();
        QMessageBox::critical(this, "Pointer Overflow", QString("Pointer overflow error: %1").arg(e.what()));
        return false;
    } catch (const CellOverflowError& e) {
        timer->stop();
        pausedAtBreakpoint = false;
        updateButtonStates();
        QMessageBox::critical(this, "Cell Overflow", QString("Cell overflow error: %1").arg(e.what()));
        return false;
    } catch (const std::exception& e) {
        timer->stop();
        pausedAtBreakpoint = false;
        updateButtonStates();
        QMessageBox::critical(this, "Runtime Error", QString("Execution error: %1").arg(e.what()));
        return false;
    }
}

void MainWindow::updateUIAfterStep() {
    try {
        QString currentOutput = QString::fromStdString(interp->getOutputBuffer());
        if (output->toPlainText() != currentOutput) {
            output->setPlainText(currentOutput);
            QTextCursor cursor = output->textCursor();
            cursor.movePosition(QTextCursor::End);
            output->setTextCursor(cursor);
        }
    } catch (...) {
       
    }

    try {
        updateStatus();
        refreshMemory();

        bool shouldHighlight = (executionMode != 2) || pausedAtBreakpoint || (!timer->isActive());
        if (shouldHighlight) {
            editor->updateHighlighting(interp->getPc());
        } else {
            editor->updateHighlighting(-1);
        }
    } catch (...) {
       
    }
}

void MainWindow::onTimer() {
    if (executionMode == 2) {
        if (!executeFastChunk()) {
            timer->stop();
            updateButtonStates();
        }
    } else {
        int stepsPerTick = (executionMode == 1) ? 1 : 10;

        for (int i = 0; i < stepsPerTick; ++i) {
            if (!interp->isRunning()) {
                timer->stop();
                updateButtonStates();
                break;
            }

            if (!executeDebugStep()) {
                break;
            }

            if (!timer->isActive()) {
                break;
            }
        }
    }
}

void MainWindow::refreshMemory() {
    int center = interp->getPointer();
    int totalRows = (interp->getMemorySize() + 15) / 16;
    int visibleRows = std::min(32, totalRows);

    int centerRow = center / 16;
    int startRow = std::max(0, centerRow - visibleRows / 2);
    int endRow = std::min(totalRows, startRow + visibleRows);

    if (endRow - startRow < visibleRows) {
        startRow = std::max(0, endRow - visibleRows);
    }

    memTable->setRowCount(endRow - startRow);

    QStringList rowLabels;
    for (int i = 0; i < endRow - startRow; ++i) {
        int addr = (startRow + i) * 16;
        rowLabels << QString("%1").arg(addr, 4, 16, QChar('0')).toUpper();
    }
    memTable->setVerticalHeaderLabels(rowLabels);

    const auto& memory = interp->getMemory();

    for (int row = 0; row < endRow - startRow; ++row) {
        for (int col = 0; col < 16; ++col) {
            int addr = (startRow + row) * 16 + col;

            auto* item = new QTableWidgetItem();

            if (addr < interp->getMemorySize()) {
                int cellValue = memory[addr];

                if (settings.cellBehavior == CellBehavior::UNLIMITED) {
                    item->setText(QString::number(cellValue));
                    if (cellValue < 0 || cellValue > 255) {
                        item->setBackground(addr == center ? QColor(255, 100, 100) : QColor(255, 240, 240));
                        item->setForeground(QColor(150, 0, 0));
                    } else if (addr == center) {
                        item->setBackground(QColor(0, 100, 0));
                        item->setForeground(QColor(255, 255, 255));
                    } else {
                        item->setBackground(QColor(255, 255, 255));
                        item->setForeground(QColor(0, 0, 0));
                    }
                } else {
                    item->setText(QString::number(cellValue));
                    if (addr == center) {
                        item->setBackground(QColor(0, 100, 0));
                        item->setForeground(QColor(255, 255, 255));
                    } else {
                        item->setBackground(QColor(255, 255, 255));
                        item->setForeground(QColor(0, 0, 0));
                    }
                }
            } else {
                item->setText("--");
                item->setBackground(QColor(240, 240, 240));
                item->setForeground(QColor(128, 128, 128));
            }

            memTable->setItem(row, col, item);
        }
    }

   
    int currentRow = center / 16 - startRow;
    if (currentRow >= 0 && currentRow < memTable->rowCount()) {
        memTable->scrollToItem(memTable->item(currentRow, center % 16));
    }

    memTable->resizeColumnsToContents();

   
    int maxWidth = 0;
    for (int i = 0; i < 16; ++i) {
        maxWidth = std::max(maxWidth, memTable->columnWidth(i));
    }
    for (int i = 0; i < 16; ++i) {
        memTable->setColumnWidth(i, maxWidth);
    }
}

void MainWindow::updateStatus() {
    QStringList modeNames = {"Debug", "Slow", "Fast"};
    QString mode = modeNames[executionMode];

    QStringList statusParts;
    statusParts << QString("[%1]").arg(mode);
    statusParts << QString("pc=%1").arg(interp->getPc());
    statusParts << QString("ptr=%1").arg(interp->getPointer());
    statusParts << QString("mem[ptr]=%1").arg(interp->getMemory()[interp->getPointer()]);
    statusParts << QString("running=%1").arg(interp->isRunning() ? "yes" : "no");

    if (pausedAtBreakpoint) {
        statusParts << "PAUSED AT BREAKPOINT";
    }

    statusParts << QString("steps=%1").arg(interp->getFastSteps());

    QMap<PointerBehavior, QString> pointerNames = {
        {PointerBehavior::CLAMP, "CLAMP"},
        {PointerBehavior::WRAP, "WRAP"},
        {PointerBehavior::ERROR, "ERROR"}
    };
    QMap<CellBehavior, QString> cellNames = {
        {CellBehavior::WRAP, "WRAP"},
        {CellBehavior::UNLIMITED, "UNLIMITED"},
        {CellBehavior::ERROR, "ERROR"}
    };

    statusParts << QString("ptr-mode=%1").arg(pointerNames[settings.pointerBehavior]);
    statusParts << QString("cell-mode=%1").arg(cellNames[settings.cellBehavior]);

    status->showMessage(statusParts.join("  "));
}

void MainWindow::loadSample() {
    QString sample = "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.";
    editor->setPlainText(sample);
    interp->reset();
    interp->loadProgram(sample.toStdString(), "");
    interp->configure(settings.pointerBehavior, settings.cellBehavior);
    interp->setInputCallback([this]() { return requestInput(); });
    editor->updateHighlighting(interp->getPc());
    refreshMemory();
    updateStatus();
    updateButtonStates();
}
