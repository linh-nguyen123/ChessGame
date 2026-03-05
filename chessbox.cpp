#include "chessbox.h"
#include "game.h"
#include <QDebug>

extern Game *game; // Reference to global game

ChessBox::ChessBox(QGraphicsItem *parent) : QGraphicsRectItem(parent)
{
    setRect(0,0,70,70); //90x90 pix
    currentPiece = nullptr;
}

void ChessBox::setColor(QColor color) {
    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(color);
    setBrush(brush);
}

void ChessBox::setOriginalColor(QColor color) {
    originalColor = color;
    setColor(color);
}

void ChessBox::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    Q_UNUSED(event);
    game->movePiece(row, col);
}
