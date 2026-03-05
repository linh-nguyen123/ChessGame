#include "game.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include "chesspiece.h"
#include <QDebug>
#include <QFont>
#include <QLabel>
#include <QGraphicsProxyWidget>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <cstring>
#include "SDL.h"
#include "databasemanager.h"
#include "SDL_mixer.h"
#include "stockfishmanager.h"
#include <QTimer>
#include <QTransform>
Game::Game(QWidget *parent) : QGraphicsView(parent)
{
    scene = new QGraphicsScene();
    scene->setSceneRect(0, 0, 800, 800);
    setScene(scene);
    setFixedSize(800, 800);


    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);

    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setRenderHint(QPainter::TextAntialiasing);

    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_AUDIO) < 0) qDebug() << "SDL Error:" << SDL_GetError();
    if (Mix_OpenAudio(48000, AUDIO_S16SYS, 2, 4096) < 0) qDebug() << "Mixer Error:" << Mix_GetError();
    Mix_AllocateChannels(32);

    QString appPath = QCoreApplication::applicationDirPath();
    moveSound = Mix_LoadWAV((appPath + "/SFX/move-self.wav").toStdString().c_str());
    castleSound = Mix_LoadWAV((appPath + "/SFX/castle.wav").toStdString().c_str());
    checkSound = Mix_LoadWAV((appPath + "/SFX/move-check.wav").toStdString().c_str());
    checkmateSound = Mix_LoadWAV((appPath + "/SFX/checkmate.wav").toStdString().c_str());
    captureSound = Mix_LoadWAV((appPath + "/SFX/capture.wav").toStdString().c_str());
    promoteSound = Mix_LoadWAV((appPath + "/SFX/promote.wav").toStdString().c_str());
    if(moveSound) Mix_VolumeChunk(moveSound, 128);
    // ... set volumes ...

    int sampleCount = 48000 * 2;
    Sint16 *noise = new Sint16[sampleCount];
    for(int i = 0; i < sampleCount; i++) noise[i] = (i % 2 == 0) ? 16 : -16;
    keepAliveChunk = Mix_QuickLoad_RAW((Uint8*)noise, sampleCount * sizeof(Sint16));
    if(keepAliveChunk) {
        Mix_ReserveChannels(1);
        Mix_PlayChannel(0, keepAliveChunk, -1);
        Mix_Volume(0, 128);
    }

    pieceToMove = nullptr;
    turn = "WHITE";
    turnLabel = nullptr;
    moveList = nullptr;
    isFlipped = false;
    isPromoting = false; // Init state

    halfMoveClock = 0;
    positionHistory.clear();
    enPassantSquare = nullptr;

    drawBoard();
    drawNotation();
    start();
    centerOn(400, 400);
    positionHistory[generateBoardState()] = 1;
    currentGameID = -1;
    moveCounter = 0;

    initializeDatabase();

    isVsAI = false;
    aiTeam = "BLACK";
    aiDifficulty = 10;

    connect(&StockfishManager::instance(),
            &StockfishManager::bestMoveReady,
            this,
            &Game::applyStockfishMove);

    StockfishManager::instance().startEngine();
}

Game::~Game() {
    Mix_HaltChannel(-1);
    Mix_FreeChunk(moveSound);
    Mix_FreeChunk(castleSound);
    Mix_FreeChunk(checkSound);
    Mix_FreeChunk(checkmateSound);
    Mix_FreeChunk(captureSound);

    Mix_FreeChunk(promoteSound);
    Mix_CloseAudio();
    SDL_Quit();
}

void Game::playSound(Mix_Chunk* sound) {
    if (sound) Mix_PlayChannel(-1, sound, 0);
    else if (moveSound) Mix_PlayChannel(-1, moveSound, 0);
}

// ... [setTurnLabel, setLogWidget, addToLog SAME AS BEFORE] ...
void Game::setTurnLabel(QLabel *label) { turnLabel = label; }
void Game::setLogWidget(QTextEdit *log) { moveList = log; }
void Game::addToLog(QString move) {
    if(!moveList) return;
    if(turn == "BLACK") moveList->insertPlainText("White: " + move + "\n");
    else {
        moveList->insertPlainText("Black: " + move + "\n");
        moveList->insertPlainText("------------------\n");
    }
    moveList->verticalScrollBar()->setValue(moveList->verticalScrollBar()->maximum());
    saveCurrentMove(move);
}

// ... [drawBoard, drawNotation, flipBoard SAME AS BEFORE] ...
void Game::drawBoard() {
    // 3. POSITIONING CONSTANTS
    int shift = 40; // Horizontally Centered
    int topGap = 0; // Top Aligned (Highest Position)

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            ChessBox *box = new ChessBox();
            collection[i][j] = box;
            box->row = i;
            box->col = j;

            // Standard White Perspective Calculation
            box->setPos(j * 70 + shift, i * 70 + topGap);

            if ((i + j) % 2 == 0) box->setOriginalColor(QColor(238, 238, 210));
            else box->setOriginalColor(QColor(118, 150, 86));
            scene->addItem(box);
        }
    }
}
void Game::drawNotation() {
    // Clear old text
    foreach(QGraphicsItem *item, notationItems) {
        scene->removeItem(item);
        delete item;
    }
    notationItems.clear();

    QColor colorCream(238, 238, 210);
    QColor colorGreen(118, 150, 86);
    QFont font("Arial", 10, QFont::Bold);

    // DRAW RANKS (Numbers 1-8)
    // We attach text to the LEFT-MOST VISUAL squares
    int colIndex = isFlipped ? 7 : 0; // Visual Left Column

    for(int i = 0; i < 8; i++) {
        QString numStr = QString::number(8 - i);
        QGraphicsTextItem *num = new QGraphicsTextItem(numStr);
        num->setFont(font);

        // Color Logic
        if ((i + colIndex) % 2 == 0) num->setDefaultTextColor(colorGreen);
        else                         num->setDefaultTextColor(colorCream);

        // Position relative to the square's new physical position
        ChessBox *box = collection[i][colIndex];
        num->setPos(box->x() + 2, box->y()); // Top-Left of square
        scene->addItem(num);
        notationItems.append(num);
    }

    // DRAW FILES (Letters a-h)
    // We attach text to the BOTTOM-MOST VISUAL squares
    int rowIndex = isFlipped ? 0 : 7; // Visual Bottom Row

    for(int j = 0; j < 8; j++) {
        QString letterStr = QChar('a' + j);
        QGraphicsTextItem *letter = new QGraphicsTextItem(letterStr);
        letter->setFont(font);

        // Color Logic
        if ((rowIndex + j) % 2 == 0) letter->setDefaultTextColor(colorGreen);
        else                         letter->setDefaultTextColor(colorCream);

        // Position relative to the square's new physical position
        ChessBox *box = collection[rowIndex][j];
        // Bottom-Right of square
        letter->setPos(box->x() + 90 - 15, box->y() + 90 -20);
        scene->addItem(letter);
        notationItems.append(letter);
    }
}
void Game::flipBoard() {
    isFlipped = !isFlipped;

    int shift = 40;
    int topGap = 0;

    // 1. Physically move every square and piece
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            ChessBox *box = collection[i][j];
            ChessPiece *piece = box->currentPiece;

            // Calculate Visual Coordinates
            int x, y;
            if (isFlipped) {
                // Black Perspective: (7,7) is at (0,0) visual
                x = (7 - j) * 70 + shift;
                y = (7 - i) * 70 + topGap;
            } else {
                // White Perspective: (0,0) is at (0,0) visual
                x = j * 70 + shift;
                y = i * 70 + topGap;
            }

            // Move Box
            box->setPos(x, y);

            // Move Piece (if present)
            if (piece) {
                // Center piece in the new box location
                qreal px = x + (70 - piece->pixmap().width()) / 2;
                qreal py = y + (70 - piece->pixmap().height()) / 2;
                piece->setPos(px, py);

                // Ensure Rotation is reset (we aren't rotating camera anymore)
                piece->setTransformOriginPoint(70/2, 70/2);
                piece->setRotation(0);
            }
        }
    }

    // 2. Redraw Notation to match new positions
    drawNotation();

    // 3. Update Promotion UI if active (or just hide it to avoid bugs)
    if (isPromoting) {
        // Simple hack: Re-run startPromotion to snap UI to new location
        foreach(QGraphicsItem *item, promotionUI) {
            scene->removeItem(item);
            delete item;
        }
        promotionUI.clear();
        startPromotion(promotionPawn->currentBox->row, promotionPawn->currentBox->col, promotionPawn->team);
    }
}
// =========================================================
// PROMOTION LOGIC (NEW)
// =========================================================

void Game::startPromotion(int row, int col, QString team) {
    QGraphicsRectItem *bg = new QGraphicsRectItem();
    bg->setRect(0, 0, 70, 280);
    bg->setBrush(QBrush(QColor(255, 255, 255, 200)));
    bg->setPen(Qt::NoPen);
    bg->setZValue(150);

    // Calculate position based on Flip State
    int shift = 40;
    int topGap = 0;

    // VISUAL COORDINATES CALCULATION
    int visualCol = isFlipped ? (7 - col) : col;
    int visualStartRow = (team == "WHITE") ? 0 : 4; // White starts top, Black starts mid

    // If flipped, Black is at top visually, White is bottom
    if (isFlipped) {
        visualStartRow = (team == "WHITE") ? 4 : 0;
    }

    bg->setPos(visualCol * 70 + shift, visualStartRow * 70 + topGap);
    scene->addItem(bg);
    promotionUI.append(bg);

    QString types[4] = {"QUEEN", "KNIGHT", "ROOK", "BISHOP"};

    for(int i = 0; i < 4; i++) {
        ChessPiece *p = new ChessPiece(team, types[i]);
        p->isPromoOption = true;
        p->setZValue(200);

        // Position logic
        int yOffset = i * 70; // Standard order down
        // If "bottom" promotion, order goes Up?
        // Let's keep it simple: List goes Down from startY

        p->setPos(visualCol * 70 + shift + 5, visualStartRow * 70 + topGap + yOffset + 5);
        scene->addItem(p);
        promotionUI.append(p);
    }
}
void Game::finalizePromotion(QString type) {
    if (!promotionPawn) return;

    // 1. Update the Pawn
    promotionPawn->type = type;
    promotionPawn->setImage();
    if(isFlipped) {
        promotionPawn->setTransformOriginPoint(40,40);
        promotionPawn->setRotation(180);
    }

    // 2. Update notation
    QString pChar;
    if (type == "KNIGHT") pChar = "N";
    else if (type == "BISHOP") pChar = "B";
    else if (type == "ROOK") pChar = "R";
    else pChar = "Q";

    pendingMoveLog += "=" + pChar;

    // 3. Clear UI
    foreach(QGraphicsItem *item, promotionUI) {
        scene->removeItem(item);
        delete item;
    }
    promotionUI.clear();
    isPromoting = false;
    promotionPawn = nullptr;

    // 4. Finish the turn
    endTurn();
}

// ... [displayGameOver, generateBoardState, checkDraw, placePiece, start, Helpers SAME AS BEFORE] ...

// =========================================================
// MOVE EXECUTION (SPLIT)
// =========================================================

void Game::movePiece(int row, int col)
{
    if(!pieceToMove || isPromoting) return;

    // --- PREPARE LOGIC ---
    QString pieceChar = "";
    if (pieceToMove->type == "KNIGHT") pieceChar = "N";
    else if (pieceToMove->type == "BISHOP") pieceChar = "B";
    else if (pieceToMove->type == "ROOK") pieceChar = "R";
    else if (pieceToMove->type == "QUEEN") pieceChar = "Q";
    else if (pieceToMove->type == "KING") pieceChar = "K";

    QString destCoord = QChar('a' + col) + QString::number(8 - row);

    bool allowed = isMoveValid(pieceToMove->currentBox->row, pieceToMove->currentBox->col, row, col, pieceToMove->team, pieceToMove->type);
    if (!allowed) {
        pieceToMove->currentBox->setOriginalColor(pieceToMove->currentBox->originalColor);
        pieceToMove = nullptr;
        return;
    }

    bool isCastling = false;
    bool isEnPassantCapture = (pieceToMove->type == "PAWN" && collection[row][col] == enPassantSquare);
    bool isPawnMove = (pieceToMove->type == "PAWN");
    bool isCapture = (collection[row][col]->currentPiece != nullptr) || isEnPassantCapture;

    // *** PROMOTION DETECTION ***
    bool willPromote = false;
    if (isPawnMove) {
        if ((pieceToMove->team == "WHITE" && row == 0) || (pieceToMove->team == "BLACK" && row == 7)) {
            willPromote = true;
        }
    }

    // Notation Part 1 (Store in pendingMoveLog)
    pendingMoveLog = "";
    if (pieceToMove->type == "KING" && abs(pieceToMove->currentBox->col - col) == 2) {
        // Castling handled below
    } else {
        if (isPawnMove) {
            if (isCapture) pendingMoveLog = QChar('a' + pieceToMove->currentBox->col) + QString("x") + destCoord;
            else pendingMoveLog = destCoord;
        } else {
            pendingMoveLog = pieceChar + (isCapture ? "x" : "") + destCoord;
        }
    }

    // Clock Updates
    if (isPawnMove || isCapture) {
        halfMoveClock = 0;
        positionHistory.clear();
    } else {
        halfMoveClock++;
    }

    ChessBox* nextEnPassantState = nullptr;
    if (pieceToMove->type == "PAWN" && abs(pieceToMove->currentBox->row - row) == 2) {
        int midRow = (pieceToMove->currentBox->row + row) / 2;
        nextEnPassantState = collection[midRow][pieceToMove->currentBox->col];
    }

    // --- EXECUTE MOVE ---
    // 1. Castling
    if (pieceToMove->type == "KING" && abs(pieceToMove->currentBox->col - col) == 2) {
        isCastling = true;
        int r = pieceToMove->currentBox->row;
        if (col > pieceToMove->currentBox->col) {
            pendingMoveLog = "O-O";
            ChessBox* rookStart = collection[r][7];
            ChessBox* rookEnd = collection[r][5];
            if(rookStart->currentPiece) {
                // ... Move rook ...
                qreal x = rookEnd->x() + (70 - rookStart->currentPiece->pixmap().width()) / 2;
                qreal y = rookEnd->y() + (70 - rookStart->currentPiece->pixmap().height()) / 2;
                rookStart->currentPiece->setPos(x, y);
                rookStart->currentPiece->currentBox = rookEnd;
                rookEnd->currentPiece = rookStart->currentPiece;
                rookStart->currentPiece = nullptr;
                rookEnd->currentPiece->hasMoved = true;
                if(isFlipped) { rookEnd->currentPiece->setTransformOriginPoint(40,40); rookEnd->currentPiece->setRotation(180); }
            }
        } else {
            pendingMoveLog = "O-O-O";
            ChessBox* rookStart = collection[r][0];
            ChessBox* rookEnd = collection[r][3];
            if(rookStart->currentPiece) {
                // ... Move rook ...
                qreal x = rookEnd->x() + (70 - rookStart->currentPiece->pixmap().width()) / 2;
                qreal y = rookEnd->y() + (70 - rookStart->currentPiece->pixmap().height()) / 2;
                rookStart->currentPiece->setPos(x, y);
                rookStart->currentPiece->currentBox = rookEnd;
                rookEnd->currentPiece = rookStart->currentPiece;
                rookStart->currentPiece = nullptr;
                rookEnd->currentPiece->hasMoved = true;
                if(isFlipped) { rookEnd->currentPiece->setTransformOriginPoint(40,40); rookEnd->currentPiece->setRotation(180); }
            }
        }
    }

    // 2. Remove Captured Piece
    ChessBox *targetBox = collection[row][col];
    if (isEnPassantCapture) {
        int victimRow = pieceToMove->currentBox->row;
        int victimCol = col;
        ChessBox* victimBox = collection[victimRow][victimCol];
        if (victimBox->currentPiece) {
            scene->removeItem(victimBox->currentPiece);
            delete victimBox->currentPiece;
            victimBox->currentPiece = nullptr;
        }
    }
    else if (targetBox->currentPiece) {
        scene->removeItem(targetBox->currentPiece);
        delete targetBox->currentPiece;
    }

    // 3. Move Physical Piece
    pieceToMove->setPos(targetBox->x() + (70-pieceToMove->pixmap().width())/2, targetBox->y() + (70-pieceToMove->pixmap().height())/2);
    pieceToMove->currentBox->currentPiece = nullptr;
    targetBox->currentPiece = pieceToMove;
    pieceToMove->currentBox = targetBox;
    pieceToMove->hasMoved = true;

    // 4. Update En Passant State
    this->enPassantSquare = nextEnPassantState;

    // 5. *** HANDLE PROMOTION PAUSE ***
    if (willPromote) {
        // PAUSE EVERYTHING
        isPromoting = true;
        promotionPawn = pieceToMove; // Store reference
        pieceToMove->currentBox->setOriginalColor(pieceToMove->currentBox->originalColor);
        pieceToMove = nullptr;

        // Show UI
        startPromotion(row, col, promotionPawn->team);
        return; // EXIT FUNCTION, wait for user input
    }

    // If not promoting, finish immediately
    endTurn();

    // Cleanup selection
    if(pieceToMove) {
        pieceToMove->currentBox->setOriginalColor(pieceToMove->currentBox->originalColor);
        pieceToMove = nullptr;
    }
}

void Game::endTurn() {
    // 1. Swap Turns
    if(turn == "WHITE") {
        turn = "BLACK";
        if(turnLabel) {
            turnLabel->setText("Turn: BLACK");
            turnLabel->setStyleSheet("background-color: #262421; color: white; border-radius: 10px; border: 2px solid white; padding: 10px; font-weight: bold; font-size: 18px;");
        }
    } else {
        turn = "WHITE";
        if(turnLabel) {
            turnLabel->setText("Turn: WHITE");
            turnLabel->setStyleSheet("background-color: #F0D9B5; color: black; border-radius: 10px; border: 2px solid black; padding: 10px; font-weight: bold; font-size: 18px;");
        }
    }

    // 2. Logic Checks
    bool isMate = checkMate(turn);
    bool isStale = checkStalemate(turn);
    bool isDraw = checkDraw();
    bool isCheck = isInCheck(turn);

    // 3. Update Log
    if (isMate) pendingMoveLog += "#";
    else if (isCheck) pendingMoveLog += "+";
    addToLog(pendingMoveLog);

    // 4. Sound & Game Over
    // FIX: return sau game over de khong goi Stockfish nua
    if (isMate) {
        QString winner = (turn == "WHITE") ? "White wins!" : "Black wins!";
        playSound(checkmateSound);
        displayGameOver("CHECKMATE!\n" + winner);
        turn = "GAMEOVER";
        return;
    }
    if (isStale) {
        playSound(checkmateSound);
        displayGameOver("STALEMATE!\nDraw!");
        turn = "GAMEOVER";
        return;
    }
    if (isDraw) {
        turn = "GAMEOVER";
        return;
    }

    // Sound binh thuong
    if      (isCheck)                        { playSound(checkSound); }
    else if (pendingMoveLog.contains("="))   { playSound(promoteSound); }
    else if (pendingMoveLog.contains("O-O")) { playSound(castleSound); }
    else if (pendingMoveLog.contains("x"))   { playSound(captureSound); }
    else                                     { playSound(moveSound); }

    // Goi Stockfish neu den luot AI
    qDebug() << "[endTurn] turn=" << turn << "| isVsAI=" << isVsAI << "| aiTeam=" << aiTeam;
    if (isVsAI && turn == aiTeam) {
        qDebug() << "[endTurn] >>> Calling Stockfish in 500ms...";
        QTimer::singleShot(500, this, [this](){
            requestStockfishMove();
        });
    }
}


void Game::displayGameOver(QString text) {
    QLabel *label = new QLabel(text);
    label->setStyleSheet(
        "background-color: rgba(38, 36, 33, 230);"
        "color: white;"
        "border: 3px solid #81b64c;"
        "border-radius: 20px;"
        "font-size: 30px;"
        "font-weight: bold;"
        "padding: 20px;"
        );
    label->setAlignment(Qt::AlignCenter);
    label->resize(400, 200);

    QGraphicsProxyWidget *proxy = scene->addWidget(label);
    proxy->setPos(200, 300);
    proxy->setZValue(100);

    if(isFlipped) {
        proxy->setTransformOriginPoint(200, 100);
        proxy->setRotation(180);
    }

    QString result;
    if (text.contains("White wins")) result = "WHITE_WIN";
    else if (text.contains("Black wins")) result = "BLACK_WIN";
    else result = "DRAW";

    DatabaseManager::instance().endGame(currentGameID, result);

    if (result == "WHITE_WIN") {
        DatabaseManager::instance().updatePlayerStats(whitePlayerID, "WIN");
        DatabaseManager::instance().updatePlayerStats(blackPlayerID, "LOSS");
    }
    else if (result == "BLACK_WIN") {
        DatabaseManager::instance().updatePlayerStats(blackPlayerID, "WIN");
        DatabaseManager::instance().updatePlayerStats(whitePlayerID, "LOSS");
    }
    else {
        DatabaseManager::instance().updatePlayerStats(whitePlayerID, "DRAW");
        DatabaseManager::instance().updatePlayerStats(blackPlayerID, "DRAW");
    }
}

QString Game::generateBoardState() {
    QString state = "";
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            ChessPiece *p = collection[i][j]->currentPiece;
            if(p) state += p->type + p->team + QString::number(i) + QString::number(j) + QString::number(p->hasMoved);
            else state += "0";
        }
    }
    if (enPassantSquare) state += "EP" + QString::number(enPassantSquare->row) + QString::number(enPassantSquare->col);
    state += turn;
    return state;
}

bool Game::checkDraw() {
    if (halfMoveClock >= 100) {
        playSound(checkmateSound);
        displayGameOver("DRAW!\n50-MOVE RULE");
        return true;
    }
    QString currentFen = generateBoardState();
    positionHistory[currentFen]++;
    if (positionHistory[currentFen] >= 3) {
        playSound(checkmateSound);
        displayGameOver("DRAW!\n3-FOLD REPETITION");
        return true;
    }
    return false;
}

void Game::placePiece(int row, int col, QString team, QString type) {
    ChessPiece *p = new ChessPiece(team, type);
    ChessBox *box = collection[row][col];
    p->currentBox = box;
    box->currentPiece = p;
    p->setPos(box->x() + (70-p->pixmap().width())/2, box->y() + (70-p->pixmap().height())/2);
    if(isFlipped) {
        p->setTransformOriginPoint(40, 40);
        p->setRotation(180);
    }
    scene->addItem(p);
}

void Game::start() {
    for(int i = 0; i < 8; i++) {
        placePiece(1, i, "BLACK", "PAWN");
        placePiece(6, i, "WHITE", "PAWN");
    }
    QString types[8] = {"ROOK", "KNIGHT", "BISHOP", "QUEEN", "KING", "BISHOP", "KNIGHT", "ROOK"};
    for(int i = 0; i < 8; i++) {
        placePiece(0, i, "BLACK", types[i]);
        placePiece(7, i, "WHITE", types[i]);
    }
}

bool Game::isInCheck(QString team) {
    int kingRow = -1, kingCol = -1;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            ChessPiece *p = collection[i][j]->currentPiece;
            if (p && p->team == team && p->type == "KING") {
                kingRow = i; kingCol = j; break;
            }
        }
    }
    if (kingRow == -1) return false;
    QString enemyTeam = (team == "WHITE") ? "BLACK" : "WHITE";
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            ChessPiece *p = collection[i][j]->currentPiece;
            if (p && p->team == enemyTeam) {
                if (canAttack(i, j, kingRow, kingCol)) return true;
            }
        }
    }
    return false;
}

bool Game::checkMate(QString team) {
    if (!isInCheck(team)) return false;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (collection[i][j]->currentPiece && collection[i][j]->currentPiece->team == team) {
                ChessPiece* p = collection[i][j]->currentPiece;
                for (int r = 0; r < 8; r++) {
                    for (int c = 0; c < 8; c++) {
                        if (isMoveValid(i, j, r, c, team, p->type)) return false;
                    }
                }
            }
        }
    }
    return true;
}

bool Game::checkStalemate(QString team) {
    if (isInCheck(team)) return false;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (collection[i][j]->currentPiece && collection[i][j]->currentPiece->team == team) {
                ChessPiece* p = collection[i][j]->currentPiece;
                for (int r = 0; r < 8; r++) {
                    for (int c = 0; c < 8; c++) {
                        if (isMoveValid(i, j, r, c, team, p->type)) return false;
                    }
                }
            }
        }
    }
    return true;
}

bool Game::canAttack(int startRow, int startCol, int endRow, int endCol) {
    ChessPiece* p = collection[startRow][startCol]->currentPiece;
    if (!p) return false;
    QString type = p->type;
    int rowDiff = abs(endRow - startRow);
    int colDiff = abs(endCol - startCol);

    if (type == "ROOK") {
        if ((rowDiff > 0 && colDiff == 0) || (colDiff > 0 && rowDiff == 0))
            return isPathClear(startRow, startCol, endRow, endCol);
    }
    else if (type == "BISHOP") {
        if (rowDiff == colDiff) return isPathClear(startRow, startCol, endRow, endCol);
    }
    else if (type == "QUEEN") {
        if (rowDiff == colDiff || rowDiff == 0 || colDiff == 0)
            return isPathClear(startRow, startCol, endRow, endCol);
    }
    else if (type == "KNIGHT") {
        if ((rowDiff == 2 && colDiff == 1) || (rowDiff == 1 && colDiff == 2)) return true;
    }
    else if (type == "KING") {
        if (rowDiff <= 1 && colDiff <= 1) return true;
    }
    else if (type == "PAWN") {
        QString team = p->team;
        int direction = (team == "WHITE") ? -1 : 1;
        if (endRow == startRow + direction && colDiff == 1) return true;
    }
    return false;
}

bool Game::isMoveValid(int startRow, int startCol, int endRow, int endCol, QString team, QString type) {
    if (this->turn != team) return false;
    if (collection[endRow][endCol]->currentPiece && collection[endRow][endCol]->currentPiece->team == team) return false;

    ChessPiece* piece = collection[startRow][startCol]->currentPiece;
    if (!piece) return false;

    int rowDiff = abs(endRow - startRow);
    int colDiff = abs(endCol - startCol);
    bool geometryValid = false;
    bool isEnPassant = (type == "PAWN" && collection[endRow][endCol] == enPassantSquare && collection[endRow][endCol]->currentPiece == nullptr);

    if (type == "ROOK") {
        if ((rowDiff > 0 && colDiff == 0) || (colDiff > 0 && rowDiff == 0)) geometryValid = isPathClear(startRow, startCol, endRow, endCol);
    }
    else if (type == "BISHOP") {
        if (rowDiff == colDiff) geometryValid = isPathClear(startRow, startCol, endRow, endCol);
    }
    else if (type == "QUEEN") {
        if ((rowDiff == colDiff) || (rowDiff == 0) || (colDiff == 0)) geometryValid = isPathClear(startRow, startCol, endRow, endCol);
    }
    else if (type == "KNIGHT") {
        if ((rowDiff == 2 && colDiff == 1) || (rowDiff == 1 && colDiff == 2)) geometryValid = true;
    }
    else if (type == "KING") {
        if (rowDiff <= 1 && colDiff <= 1) geometryValid = true;
        if (rowDiff == 0 && colDiff == 2 && !piece->hasMoved) {
            if (isInCheck(team)) return false;
            if (endCol > startCol) {
                ChessPiece* rook = collection[startRow][7]->currentPiece;
                if (rook && rook->type == "ROOK" && rook->team == team && !rook->hasMoved) {
                    if (!collection[startRow][5]->currentPiece && !collection[startRow][6]->currentPiece) geometryValid = true;
                }
            } else {
                ChessPiece* rook = collection[startRow][0]->currentPiece;
                if (rook && rook->type == "ROOK" && rook->team == team && !rook->hasMoved) {
                    if (!collection[startRow][1]->currentPiece && !collection[startRow][2]->currentPiece && !collection[startRow][3]->currentPiece) geometryValid = true;
                }
            }
        }
    }
    else if (type == "PAWN") {
        int direction = (team == "WHITE") ? -1 : 1;
        int startRank = (team == "WHITE") ? 6 : 1;
        if (colDiff == 0 && endRow == startRow + direction && !collection[endRow][endCol]->currentPiece) geometryValid = true;
        if (colDiff == 0 && endRow == startRow + (2 * direction) && startRow == startRank) {
            if (!collection[endRow][endCol]->currentPiece && isPathClear(startRow, startCol, endRow, endCol)) geometryValid = true;
        }
        if (colDiff == 1 && endRow == startRow + direction && collection[endRow][endCol]->currentPiece) geometryValid = true;
        if (colDiff == 1 && endRow == startRow + direction && isEnPassant) geometryValid = true;
    }

    if (!geometryValid) return false;

    ChessBox* startBox = collection[startRow][startCol];
    ChessBox* targetBox = collection[endRow][endCol];
    ChessPiece* capturedPiece = targetBox->currentPiece;
    ChessBox* originalBoxPtr = piece->currentBox;

    ChessBox* epVictimBox = nullptr;
    ChessPiece* epVictimPiece = nullptr;
    if (isEnPassant) {
        epVictimBox = collection[startRow][endCol];
        epVictimPiece = epVictimBox->currentPiece;
    }

    startBox->currentPiece = nullptr;
    targetBox->currentPiece = piece;
    piece->currentBox = targetBox;
    if (isEnPassant && epVictimBox) epVictimBox->currentPiece = nullptr;

    bool kingInDanger = isInCheck(team);

    targetBox->currentPiece = capturedPiece;
    startBox->currentPiece = piece;
    piece->currentBox = originalBoxPtr;
    if (isEnPassant && epVictimBox) epVictimBox->currentPiece = epVictimPiece;

    if (kingInDanger) return false;

    return true;
}

bool Game::isPathClear(int startRow, int startCol, int endRow, int endCol) {
    int dRow = (endRow > startRow) ? 1 : (endRow < startRow) ? -1 : 0;
    int dCol = (endCol > startCol) ? 1 : (endCol < startCol) ? -1 : 0;
    int r = startRow + dRow;
    int c = startCol + dCol;
    while (r != endRow || c != endCol) {
        if (collection[r][c]->currentPiece) return false;
        r += dRow;
        c += dCol;
    }
    return true;
}

void Game::initializeDatabase()
{
    if (DatabaseManager::instance().connectToDatabase()) {
        // Tạo 2 player mặc định hoặc lấy từ input
        whitePlayerID = DatabaseManager::instance().addPlayer("White Player");
        blackPlayerID = DatabaseManager::instance().addPlayer("Black Player");

        // Bắt đầu game mới
        currentGameID = DatabaseManager::instance().startNewGame(whitePlayerID, blackPlayerID);

        qDebug() << "Game ID:" << currentGameID;
    }
}

void Game::saveCurrentMove(const QString &move)
{
    if (currentGameID > 0) {
        moveCounter++;
        DatabaseManager::instance().saveMove(currentGameID, moveCounter, move, turn);
    }
}

QString Game::generateFEN()
{
    QString fen = "";

    // 1. VỊ TRÍ CÁC QUÂN CỜ
    for (int row = 0; row < 8; row++) {
        int emptyCount = 0;

        for (int col = 0; col < 8; col++) {
            ChessPiece *piece = collection[row][col]->currentPiece;

            if (!piece) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen += QString::number(emptyCount);
                    emptyCount = 0;
                }

                QString symbol;
                if (piece->type == "KING")   symbol = "k";
                else if (piece->type == "QUEEN")  symbol = "q";
                else if (piece->type == "ROOK")   symbol = "r";
                else if (piece->type == "BISHOP") symbol = "b";
                else if (piece->type == "KNIGHT") symbol = "n";
                else if (piece->type == "PAWN")   symbol = "p";

                // White = uppercase, Black = lowercase
                if (piece->team == "WHITE") symbol = symbol.toUpper();

                fen += symbol;
            }
        }

        if (emptyCount > 0) fen += QString::number(emptyCount);
        if (row < 7) fen += "/";
    }

    // 2. LƯỢT ĐI (w = white, b = black)
    fen += (turn == "WHITE") ? " w " : " b ";

    // 3. QUYỀN NHẬP THÀNH (đơn giản hóa)
    fen += "KQkq ";

    // 4. EN PASSANT
    fen += "- ";

    // 5. HALF MOVE CLOCK & FULL MOVE
    fen += QString::number(halfMoveClock) + " ";
    fen += QString::number(moveCounter / 2 + 1);

    qDebug() << "FEN:" << fen;
    return fen;
}

void Game::requestStockfishMove()
{
    qDebug() << "[requestStockfishMove] isVsAI=" << isVsAI << "| turn=" << turn << "| aiTeam=" << aiTeam;

    if (!isVsAI) { qDebug() << "  SKIP: not AI mode"; return; }
    if (turn != aiTeam) { qDebug() << "  SKIP: not AI turn"; return; }
    if (turn == "GAMEOVER") { qDebug() << "  SKIP: game over"; return; }

    if (!StockfishManager::instance().isRunning()) {
        qDebug() << "  ERROR: Stockfish not running! Retrying...";
        if (!StockfishManager::instance().startEngine()) {
            qDebug() << "  FATAL: Cannot start Stockfish!";
            return;
        }
    }

    int thinkTime = 500;
    if (aiDifficulty >= 6)  thinkTime = 1000;
    if (aiDifficulty >= 11) thinkTime = 2000;

    QString fen = generateFEN();
    qDebug() << "  FEN:" << fen;
    StockfishManager::instance().getBestMove(fen, thinkTime);
}

void Game::applyStockfishMove(QString move)
{
    qDebug() << "[applyStockfishMove] move=" << move;

    if (move.length() < 4) {
        qDebug() << "  ERROR: move too short!";
        return;
    }

    // Chuyen tu ky hieu co vua sang toa do
    // Vi du: "e2e4" -> from(row=6,col=4) to(row=4,col=4)
    int fromCol = move[0].toLatin1() - 'a';   // e=4
    int fromRow = 8 - move[1].digitValue();    // 2 -> row 6
    int toCol   = move[2].toLatin1() - 'a';   // e=4
    int toRow   = 8 - move[3].digitValue();    // 4 -> row 4

    qDebug() << "  from=(" << fromRow << "," << fromCol
             << ") to=(" << toRow << "," << toCol << ")";

    // Kiem tra toa do hop le
    if (fromRow < 0 || fromRow > 7 || fromCol < 0 || fromCol > 7 ||
        toRow   < 0 || toRow   > 7 || toCol   < 0 || toCol   > 7) {
        qDebug() << "  ERROR: coordinates out of range!";
        return;
    }

    ChessPiece *piece = collection[fromRow][fromCol]->currentPiece;
    if (!piece) {
        qDebug() << "  ERROR: no piece at source (" << fromRow << "," << fromCol << ")!";
        return;
    }

    qDebug() << "  Moving:" << piece->type << piece->team;

    // Dat piece can move truoc khi goi movePiece
    pieceToMove = piece;
    movePiece(toRow, toCol);

    // Xu ly phong cap
    if (move.length() == 5) {
        QString promoType;
        switch (move[4].toLatin1()) {
        case 'q': promoType = "QUEEN";  break;
        case 'r': promoType = "ROOK";   break;
        case 'b': promoType = "BISHOP"; break;
        case 'n': promoType = "KNIGHT"; break;
        }
        if (!promoType.isEmpty()) {
            qDebug() << "  Promotion to:" << promoType;
            finalizePromotion(promoType);
        }
    }
}
