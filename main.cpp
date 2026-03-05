#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QDialog>
#include <QDebug>           // FIX 2: Thêm QDebug
#include "game.h"
#include "databasemanager.h"
#include "stockfishmanager.h"   // FIX 1: Thêm include StockfishManager
#include <QShortcut>
#include <QKeySequence>
#include <QScrollBar>

Game *game;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // FIX 6: Không gọi connectToDatabase ở đây vì
    // initializeDatabase() trong Game constructor đã gọi rồi
    // → Tránh lỗi "duplicate connection name"

    // 1. Create Main Window
    QWidget *mainWindow = new QWidget();
    mainWindow->setWindowTitle("UTE Chess");
    mainWindow->setStyleSheet("background-color: #312E2B;");

    // 2. ROOT LAYOUT
    QVBoxLayout *rootLayout = new QVBoxLayout(mainWindow);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ============================================================
    // A. HEADER
    // ============================================================
    QFrame *headerFrame = new QFrame();
    headerFrame->setFixedHeight(80);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(20, 0, 0, 0);

    QLabel *logoLabel = new QLabel();
    QPixmap logo(":/image/images/ChessLogo.png");
    logoLabel->setPixmap(logo.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    headerLayout->addWidget(logoLabel);
    headerLayout->addStretch();
    rootLayout->addWidget(headerFrame);

    // ============================================================
    // B. MAIN CONTENT
    // ============================================================
    QWidget *mainArea = new QWidget();
    QHBoxLayout *mainLayout = new QHBoxLayout(mainArea);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(5);

    // --- 1. BOARD COLUMN ---
    QVBoxLayout *boardColumn = new QVBoxLayout();
    boardColumn->setSpacing(10);

    QLabel *opLogo = new QLabel();
    QPixmap opPix(":/image/images/black_400@2x.png");
    opLogo->setPixmap(opPix.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    game = new Game();
    game->setFixedSize(800, 800);

    boardColumn->addWidget(opLogo, 0, Qt::AlignLeft);
    boardColumn->addWidget(game);

    mainLayout->addLayout(boardColumn);

    // --- 2. SIDE PANEL ---
    QFrame *sidePanel = new QFrame();
    sidePanel->setFixedWidth(350);
    sidePanel->setStyleSheet("background-color: #262421; border-radius: 8px; margin-right: 20px; margin-top: 20px;");

    QVBoxLayout *sideLayout = new QVBoxLayout(sidePanel);

    QPushButton *flipBtn = new QPushButton("Flip Board");
    flipBtn->setStyleSheet(
        "QPushButton { background-color: #45423D; color: #C0C0C0; border-radius: 5px; padding: 10px; font-weight: bold; }"
        "QPushButton:hover { background-color: #605D58; color: white; }"
        );
    QObject::connect(flipBtn, &QPushButton::clicked, [](){
        game->flipBoard();
    });

    QLabel *turnLabel = new QLabel("Turn: WHITE");
    turnLabel->setAlignment(Qt::AlignCenter);
    turnLabel->setFixedHeight(60);
    turnLabel->setStyleSheet("background-color: #F0D9B5; color: black; border-radius: 8px; font-weight: bold; font-size: 18px;");

    QTextEdit *moveLog = new QTextEdit();
    moveLog->setReadOnly(true);
    moveLog->setStyleSheet(
        "QTextEdit { background-color: #21201D; color: white; border: none; border-radius: 5px; padding: 10px; font-size: 16px; }"
        );

    // FIX 4: Gán turnLabel và moveLog TRƯỚC khi hiện dialog
    // → Tránh NULL pointer nếu AI đi ngay sau khi chọn mode
    game->setTurnLabel(turnLabel);
    game->setLogWidget(moveLog);

    // ============================================================
    // C. MODE SELECTION DIALOG
    // ============================================================
    QDialog *modeDialog = new QDialog();
    modeDialog->setWindowTitle("Selection mode");
    modeDialog->setFixedSize(350, 280);
    modeDialog->setStyleSheet("background-color: #312E2B;");

    QVBoxLayout *mLayout = new QVBoxLayout(modeDialog);

    // FIX 3: Dùng text thuần thay vì emoji (tránh lỗi encoding)
    QLabel *title = new QLabel("UTE CHESS");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("color:white; font-size:22px; font-weight:bold;");

    QPushButton *pvpBtn    = new QPushButton("local player");
    QPushButton *aiEasyBtn = new QPushButton("player vs ai - easy (Level 3)");
    QPushButton *aiMedBtn  = new QPushButton("player vs ai - medium (Level 10)");
    QPushButton *aiHardBtn = new QPushButton("player vs ai - hard (Level 20)");

    QString btnStyle = "QPushButton { color:white; padding:12px; "
                       "border-radius:6px; font-size:14px; margin:3px; }";

    pvpBtn->setStyleSheet(btnStyle + "QPushButton { background:#45423D; }");
    aiEasyBtn->setStyleSheet(btnStyle + "QPushButton { background:#2C6F2C; }");
    aiMedBtn->setStyleSheet(btnStyle + "QPushButton { background:#8F6F1A; }");
    aiHardBtn->setStyleSheet(btnStyle + "QPushButton { background:#8F2C2C; }");

    // Local PvP
    QObject::connect(pvpBtn, &QPushButton::clicked, [&](){
        game->isVsAI = false;
        modeDialog->accept();
    });

    // AI Easy (Level 3)
    QObject::connect(aiEasyBtn, &QPushButton::clicked, [&](){
        game->isVsAI = true;
        game->aiTeam = "BLACK";
        game->aiDifficulty = 3;
        StockfishManager::instance().setDifficulty(3);   // FIX 1: Cần include stockfishmanager.h
        modeDialog->accept();
    });

    // AI Medium (Level 10)
    QObject::connect(aiMedBtn, &QPushButton::clicked, [&](){
        game->isVsAI = true;
        game->aiTeam = "BLACK";
        game->aiDifficulty = 10;
        StockfishManager::instance().setDifficulty(10);
        modeDialog->accept();
    });

    // AI Hard (Level 20)
    QObject::connect(aiHardBtn, &QPushButton::clicked, [&](){
        game->isVsAI = true;
        game->aiTeam = "BLACK";
        game->aiDifficulty = 20;
        StockfishManager::instance().setDifficulty(20);
        modeDialog->accept();
    });

    mLayout->addWidget(title);
    mLayout->addWidget(pvpBtn);
    mLayout->addWidget(aiEasyBtn);
    mLayout->addWidget(aiMedBtn);
    mLayout->addWidget(aiHardBtn);

    // FIX 4: Dialog exec() SAU KHI đã gán turnLabel và moveLog
    modeDialog->exec();

    // Add widgets to side layout
    sideLayout->addWidget(turnLabel);
    sideLayout->addWidget(flipBtn);
    sideLayout->addWidget(moveLog);

    mainLayout->addWidget(sidePanel);

    // ============================================================
    // FINALIZE
    // ============================================================
    rootLayout->addWidget(mainArea);
    rootLayout->addStretch();

    auto toggleFullscreen = [mainWindow](){
        if(mainWindow->isFullScreen()) mainWindow->showNormal();
        else mainWindow->showFullScreen();
    };
    QObject::connect(new QShortcut(QKeySequence(Qt::Key_F11), mainWindow), &QShortcut::activated, toggleFullscreen);

    mainWindow->showFullScreen();
    return a.exec();
}
