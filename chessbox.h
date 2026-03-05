#ifndef CHESSBOX_H
#define CHESSBOX_H

#include <QGraphicsRectItem>
#include <QBrush>
#include <QGraphicsSceneMouseEvent>

class ChessPiece; // Forward declaration

class ChessBox : public QGraphicsRectItem
{
public:
    ChessBox(QGraphicsItem *parent = nullptr);

    // Color of the square (Green or Cream)
    void setColor(QColor color);
    void setOriginalColor(QColor color);
    QColor originalColor;

    // Location on the board
    int row;
    int col;

    // Logic: Is there a piece here?
    ChessPiece * currentPiece;

protected:
    // Event: Detect clicks
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
};

#endif // CHESSBOX_H
