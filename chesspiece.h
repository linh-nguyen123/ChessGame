#ifndef CHESSPIECE_H
#define CHESSPIECE_H

#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include "chessbox.h"

class ChessPiece : public QGraphicsPixmapItem
{
public:
    ChessPiece(QString team, QString type, QGraphicsItem *parent = nullptr);
    void setImage();

    bool hasMoved;
    QString team;
    QString type;
    ChessBox *currentBox;
    QPointF originalPos;

    // --- NEW: Promotion Selector Flag ---
    bool isPromoOption;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
};

#endif // CHESSPIECE_H
