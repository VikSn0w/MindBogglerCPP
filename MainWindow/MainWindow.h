#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QTextEdit>
#include <QtCore/QTimer>
#include <QtGui/QAction>
#include <QtGui/QTextCharFormat>
#include <QtGui/QTextCursor>
#include "../INTERPRETER/Interpreter.h"
#include <set>

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

    private:
        std::set<int> breakpointIndices;
        QColor currentColor;
        QColor breakpointColor;

    public:
        explicit CodeEditor(QWidget* parent = nullptr);
        void updateHighlighting(int currentPc);
        const std::set<int>& getBreakpoints() const { return breakpointIndices; }

    public slots:
        void toggleBreakpointAtCaret();
};

class SettingsDialog : public QDialog {
    Q_OBJECT

    private:
        QButtonGroup* pointerBehaviorGroup;
        QButtonGroup* cellBehaviorGroup;
        QRadioButton* clampRadio;
        QRadioButton* wrapRadio;
        QRadioButton* errorRadio;
        QRadioButton* cellWrapRadio;
        QRadioButton* cellUnlimitedRadio;
        QRadioButton* cellErrorRadio;
        QPushButton* okButton;
        QPushButton* cancelButton;

    public:
        explicit SettingsDialog(QWidget* parent = nullptr);

        PointerBehavior getPointerBehavior() const;
        void setPointerBehavior(PointerBehavior behavior);
        CellBehavior getCellBehavior() const;
        void setCellBehavior(CellBehavior behavior);
};

class CompileOutputDialog : public QDialog {
    Q_OBJECT

    private:
        QPlainTextEdit* textArea;

    public:
        explicit CompileOutputDialog(QWidget* parent = nullptr,
                                   const QString& title = "Compilation Results",
                                   const QString& content = "");
};

class AboutDialog : public QDialog {
    Q_OBJECT

    public:
        explicit AboutDialog(QWidget* parent = nullptr);
};

class MainWindow : public QMainWindow {
    Q_OBJECT

    private:
        std::unique_ptr<Interpreter> interp;
        QTimer* timer;

        CodeEditor* editor;
        QPlainTextEdit* output;
        QTableWidget* memTable;

        QPushButton* btnRun;
        QPushButton* btnStep;
        QPushButton* btnPause;
        QPushButton* btnResume;
        QPushButton* btnReset;
        QPushButton* btnClearOut;

        QRadioButton* modeDebug;
        QRadioButton* modeSlow;
        QRadioButton* modeFast;
        QButtonGroup* modeGroup;

        QAction* actOpen;
        QAction* actSave;
        QAction* actCheck;
        QAction* actBreak;
        QAction* actCompile;
        QAction* actPseudocode;
        QAction* actSettings;
        QAction* actAbout;

        QStatusBar* status;

        struct Settings {
            PointerBehavior pointerBehavior = PointerBehavior::CLAMP;
            CellBehavior cellBehavior = CellBehavior::WRAP;
        } settings;

        int executionMode;
        bool pausedAtBreakpoint;
        std::map<int, int> timerIntervals;

        void buildUI();
        void connectActions();
        void loadInterpreterFromUI();
        bool executeOneStep();
        bool executeFastChunk();
        bool executeDebugStep();
        void updateUIAfterStep();
        void refreshMemory();
        void updateStatus();
        void updateButtonStates();
        void loadSample();
        std::string requestInput();
        std::string generatePseudocodeFallback(const std::string& program);

    private slots:
        void onTimer();
        void onModeChanged();
        void onRun();
        void onStep();
        void onPause();
        void onResume();
        void onReset();
        void onOpen();
        void onSave();
        void onCheck();
        void onSettings();
        void onCompile();
        void onPseudocode();
        void onAbout();

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() = default;
};



#endif //MAINWINDOW_H
