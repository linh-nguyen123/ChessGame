#ifndef STOCKFISHMANAGER_H
#define STOCKFISHMANAGER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

class StockfishManager : public QObject
{
    Q_OBJECT
public:
    static StockfishManager& instance();

    bool startEngine();
    void stopEngine();
    bool isRunning();

    // Gửi vị trí bàn cờ và yêu cầu tính toán
    void getBestMove(const QString &fen, int thinkTimeMs = 1000);

    // Cài độ khó (0-20, 20 là mạnh nhất)
    void setDifficulty(int level);

signals:
    // Trả về nước đi tốt nhất dạng "e2e4"
    void bestMoveReady(QString move);

private slots:
    void onDataReceived();

private:
    StockfishManager();
    ~StockfishManager();

    void sendCommand(const QString &cmd);

    QProcess *process;
    int difficulty;
};

#endif // STOCKFISHMANAGER_H
