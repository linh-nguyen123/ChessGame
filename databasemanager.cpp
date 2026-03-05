#include "databasemanager.h"
#include <QDebug>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() : isConnected(false)
{
}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
}

bool DatabaseManager::connectToDatabase()
{
    db = QSqlDatabase::addDatabase("QODBC");

    // Cấu hình connection string
    db.setDatabaseName("DRIVER={ODBC Driver 17 for SQL Server};"
                       "SERVER=localhost\\SQLEXPRESS;"
                       "DATABASE=ChessGameDB;"
                       "Trusted_Connection=yes;");

    if (!db.open()) {
        qDebug() << "Database connection error:" << db.lastError().text();
        isConnected = false;
        return false;
    }

    qDebug() << "Database connected successfully!";
    isConnected = true;
    return true;
}

void DatabaseManager::closeDatabase()
{
    if (isConnected) {
        db.close();
        isConnected = false;
    }
}

int DatabaseManager::addPlayer(const QString &playerName)
{
    QSqlQuery query;
    query.prepare("INSERT INTO Players (PlayerName) VALUES (?); SELECT SCOPE_IDENTITY();");
    query.addBindValue(playerName);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    qDebug() << "Error adding player:" << query.lastError().text();
    return -1;
}

QString DatabaseManager::getPlayerName(int playerID)
{
    QSqlQuery query;
    query.prepare("SELECT PlayerName FROM Players WHERE PlayerID = ?");
    query.addBindValue(playerID);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }

    return QString();
}

int DatabaseManager::startNewGame(int whitePlayerID, int blackPlayerID)
{
    QSqlQuery query;
    query.prepare("INSERT INTO Games (WhitePlayerID, BlackPlayerID, Result) "
                  "VALUES (?, ?, 'ONGOING'); SELECT SCOPE_IDENTITY();");
    query.addBindValue(whitePlayerID);
    query.addBindValue(blackPlayerID);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    qDebug() << "Error creating game:" << query.lastError().text();
    return -1;
}

void DatabaseManager::endGame(int gameID, const QString &result)
{
    QSqlQuery query;
    query.prepare("UPDATE Games SET Result = ?, EndTime = GETDATE() WHERE GameID = ?");
    query.addBindValue(result);
    query.addBindValue(gameID);

    if (!query.exec()) {
       qDebug() << "Error ending game:" << query.lastError().text();
    }
}

void DatabaseManager::saveMove(int gameID, int moveNumber, const QString &moveNotation, const QString &team)
{
    QSqlQuery query;
    query.prepare("INSERT INTO Moves (GameID, MoveNumber, MoveNotation, Team) VALUES (?, ?, ?, ?)");
    query.addBindValue(gameID);
    query.addBindValue(moveNumber);
    query.addBindValue(moveNotation);
    query.addBindValue(team);

    if (!query.exec()) {
       qDebug() << "Error saving move:" << query.lastError().text();
    }
}

QStringList DatabaseManager::getGameMoves(int gameID)
{
    QStringList moves;
    QSqlQuery query;
    query.prepare("SELECT MoveNotation FROM Moves WHERE GameID = ? ORDER BY MoveNumber");
    query.addBindValue(gameID);

    if (query.exec()) {
        while (query.next()) {
            moves.append(query.value(0).toString());
        }
    }

    return moves;
}

void DatabaseManager::updatePlayerStats(int playerID, const QString &result)
{
    QSqlQuery query;

    // Kiểm tra xem player đã có stats chưa
    query.prepare("SELECT StatID FROM PlayerStats WHERE PlayerID = ?");
    query.addBindValue(playerID);

    if (!query.exec() || !query.next()) {
        // Tạo mới
        query.prepare("INSERT INTO PlayerStats (PlayerID) VALUES (?)");
        query.addBindValue(playerID);
        query.exec();
    }

    // Cập nhật thống kê
    QString updateField;
    if (result == "WIN") updateField = "Wins = Wins + 1";
    else if (result == "LOSS") updateField = "Losses = Losses + 1";
    else if (result == "DRAW") updateField = "Draws = Draws + 1";

    query.prepare(QString("UPDATE PlayerStats SET %1 WHERE PlayerID = ?").arg(updateField));
    query.addBindValue(playerID);
    query.exec();
}

QMap<QString, int> DatabaseManager::getPlayerStats(int playerID)
{
    QMap<QString, int> stats;
    QSqlQuery query;
    query.prepare("SELECT Wins, Losses, Draws FROM PlayerStats WHERE PlayerID = ?");
    query.addBindValue(playerID);

    if (query.exec() && query.next()) {
        stats["Wins"] = query.value(0).toInt();
        stats["Losses"] = query.value(1).toInt();
        stats["Draws"] = query.value(2).toInt();
    }

    return stats;
}
