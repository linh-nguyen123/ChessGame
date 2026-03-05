#include "chesspiece.h"
#include "game.h"
#include <QPixmap>
#include <QDebug>

extern Game *game;

ChessPiece::ChessPiece(QString team, QString type, QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent), team(team), type(type)
{
    isPromoOption = false; // Default
    setImage();

    // Enable mouse dragging
    setFlag(QGraphicsItem::ItemIsMovable);
    setZValue(1);
}

void ChessPiece::setImage() {
    QPixmap image;

    if (team == "WHITE") {
        if(type == "ROOK")   image.load(":/image/images/white-rook.png");
        if(type == "KING")   image.load(":/image/images/white-king.png");
        if(type == "QUEEN")  image.load(":/image/images/white-queen.png");
        if(type == "KNIGHT") image.load(":/image/images/white-knight.png");
        if(type == "BISHOP") image.load(":/image/images/white-bishop.png");
        if(type == "PAWN")   image.load(":/image/images/white-pawn.png");
    } else {
        if(type == "ROOK")   image.load(":/image/images/black-rook.png");
        if(type == "KING")   image.load(":/image/images/black-king.png");
        if(type == "QUEEN")  image.load(":/image/images/black-queen.png");
        if(type == "KNIGHT") image.load(":/image/images/black-knight.png");
        if(type == "BISHOP") image.load(":/image/images/black-bishop.png");
        if(type == "PAWN")   image.load(":/image/images/black-pawn.png");
    }

    setPixmap(image.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void ChessPiece::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // 1. Handle Promotion (Special Case)
    if (isPromoOption) {
        game->finalizePromotion(this->type);
        return;
    }

    // 2. DEBUG & CHECK
    qDebug() << "I clicked a piece!";
    qDebug() << "My Team: " << this->team;
    qDebug() << "Game Turn: " << game->turn;

    // IF THIS IS AT THE TOP, THE DRAG STARTS IMMEDIATELY.
    // IT MUST BE MOVED TO THE BOTTOM.

    // --- CORRECT LOGIC ---
    if (this->team != game->turn) {
        qDebug() << "Not my turn! Click ignored.";
        event->ignore(); // Add this to be safe (passes click to board for capturing)
        return;          // STOP HERE. Do not go further.
    }

    if (game->isPromoting) return;

    // 3. Select the piece
    game->pieceToMove = this;

    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            game->collection[i][j]->setOriginalColor(game->collection[i][j]->originalColor);
        }
    }

    if(currentBox) {
        currentBox->setColor(Qt::red);
    }

    originalPos = this->pos();
    setZValue(10);

    // 4. START DRAG (MUST BE LAST)
    // Only call this if we passed the checks above!
    QGraphicsPixmapItem::mousePressEvent(event);
}

void ChessPiece::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (isPromoOption || game->isPromoting) {
        QGraphicsPixmapItem::mouseReleaseEvent(event);
        return;
    }

    int shiftX = 40;
    int shiftY = 0;

    qreal dropX = pos().x() + pixmap().width() / 2;
    qreal dropY = pos().y() + pixmap().height() / 2;

    // 1. Calculate VISUAL grid coordinates
    int visCol = static_cast<int>(dropX - shiftX) / 70;
    int visRow = static_cast<int>(dropY - shiftY) / 70;

    // 2. Convert to LOGICAL coordinates
    int col, row;
    if (game->isFlipped) {
        col = 7 - visCol;
        row = 7 - visRow;
    } else {
        col = visCol;
        row = visRow;
    }

    // 3. Validate
    if (col < 0 || col > 7 || row < 0 || row > 7) {
        setPos(originalPos);
    }
    else {
        ChessBox* startBox = currentBox;
        if (startBox->col != col || startBox->row != row) {
            game->movePiece(row, col);
        }
        if (currentBox == startBox) {
            setPos(originalPos);
        }
    }

    setZValue(1);
    QGraphicsPixmapItem::mouseReleaseEvent(event);
}
