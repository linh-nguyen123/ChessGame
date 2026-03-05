#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDateTime>
#include <QMap>

class DatabaseManager
{
public:
    static DatabaseManager& instance();

    bool connectToDatabase();
    void closeDatabase();

    int addPlayer(const QString &playerName);
    QString getPlayerName(int PlayerID);

    int startNewGame(int whitePlayerID, int blackPlayerID);
    void endGame(int gameID, const QString &result);

    void saveMove(int gameID, int moveNumber, const QString &moveNotation, const QString &team);
    QStringList getGameMoves(int gameID);

    void updatePlayerStats(int playerID, const QString &result);
    QMap<QString, int> getPlayerStats(int playerID);

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase db;
    bool isConnected;
};

#endif // DATABASEMANAGER_H
