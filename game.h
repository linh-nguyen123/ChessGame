#ifndef GAME_H
#define GAME_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLabel>
#include <QTextEdit>
#include <QMap>
#include "chessbox.h"

struct Mix_Chunk;
class ChessPiece;

class Game : public QGraphicsView
{
    Q_OBJECT
public:
    Game(QWidget *parent = nullptr);
    ~Game();

    // --- UI SETUP ---
    void setTurnLabel(QLabel *label);
    void setLogWidget(QTextEdit *log);

    void drawBoard();
    void drawNotation();
    void start();
    void placePiece(int row, int col, QString team, QString type);
    void movePiece(int row, int col);
    void playSound(Mix_Chunk* sound);

    // --- NEW FEATURES ---
    void flipBoard();
    void addToLog(QString move);

    // --- PROMOTION LOGIC (NEW) ---
    void startPromotion(int row, int col, QString team);
    void finalizePromotion(QString type);
    void endTurn(); // Separated logic

    // --- UI FUNCTIONS ---
    void displayGameOver(QString text);

    // --- DRAW LOGIC ---
    QString generateBoardState();
    bool checkDraw();

    // --- LOGIC HELPERS ---
    bool isInCheck(QString team);
    bool checkMate(QString team);
    bool checkStalemate(QString team);
    bool canAttack(int startRow, int startCol, int endRow, int endCol);
    bool isMoveValid(int startRow, int startCol, int endRow, int endCol, QString team, QString type);
    bool isPathClear(int startRow, int startCol, int endRow, int endCol);

    // Variables
    QLabel *turnLabel;
    QTextEdit *moveList;
    bool isFlipped;

    // --- PROMOTION STATE ---
    bool isPromoting;
    ChessPiece *promotionPawn; // The pawn waiting to transform
    QString pendingMoveLog;    // Store "a8" so we can append "=Q" later
    QList<QGraphicsItem*> promotionUI; // To clear overlay

    // --- DRAW VARIABLES ---
    int halfMoveClock;
    QMap<QString, int> positionHistory;
    QList<QGraphicsItem*> notationItems;

    ChessBox *enPassantSquare;

    // --- AUDIO VARIABLES ---
    Mix_Chunk *moveSound;
    Mix_Chunk *castleSound;
    Mix_Chunk *checkSound;
    Mix_Chunk *checkmateSound;
    Mix_Chunk *captureSound;
    Mix_Chunk *keepAliveChunk;
    Mix_Chunk *promoteSound;
    QString turn;
    ChessPiece *pieceToMove;
    QGraphicsScene *scene;
    ChessBox *collection[8][8];

public:
    int currentGameID;
    int moveCounter;
    int whitePlayerID;
    int blackPlayerID;

    void initializeDatabase();
    void saveCurrentMove(const QString &move);
    QString generateFEN();           // Tạo FEN từ bàn cờ hiện tại
    void applyStockfishMove(QString move);  // Thực hiện nước đi từ Stockfish
    void requestStockfishMove();     // Yêu cầu Stockfish tính toán

    bool isVsAI;                     // Chơi với AI hay không
    QString aiTeam;                  // AI chơi team nào ("BLACK" thường)
    int aiDifficulty;                // Độ khó 0-20
};

#endif // GAME_H
