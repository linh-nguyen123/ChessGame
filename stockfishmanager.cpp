#include "stockfishmanager.h"
#include <QDebug>
#include <QCoreApplication>
#include <QFileInfo>

StockfishManager& StockfishManager::instance()
{
    static StockfishManager instance;
    return instance;
}

StockfishManager::StockfishManager() : difficulty(10)
{
    process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardOutput,
            this, &StockfishManager::onDataReceived);

    // FIX: Bat loi neu Stockfish crash
    connect(process, &QProcess::errorOccurred,
            this, [](QProcess::ProcessError error){
                qDebug() << "[Stockfish] Process error:" << error;
            });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [](int code, QProcess::ExitStatus status){
                qDebug() << "[Stockfish] Process finished. Code:" << code << "Status:" << status;
            });
}

StockfishManager::~StockfishManager()
{
    stopEngine();
}

bool StockfishManager::startEngine()
{
    // BUOC 1: Tim duong dan den stockfish.exe
    // stockfish.exe phai dat tai: <build_folder>/debug/stockfish/stockfish.exe
    QString appDir = QCoreApplication::applicationDirPath();
    QString path = appDir + "/stockfish/stockfish.exe";

    // FIX: In ra duong dan de de kiem tra
    qDebug() << "[Stockfish] Looking for exe at:" << path;
    qDebug() << "[Stockfish] App dir:" << appDir;

    // FIX: Kiem tra file ton tai truoc khi chay
    if (!QFileInfo::exists(path)) {
        qDebug() << "[Stockfish] ERROR: stockfish.exe NOT FOUND!";
        qDebug() << "[Stockfish] Please place stockfish.exe at:" << path;
        return false;
    }

    qDebug() << "[Stockfish] Found exe, starting...";
    process->start(path);

    if (!process->waitForStarted(5000)) {
        qDebug() << "[Stockfish] ERROR: Failed to start process!";
        qDebug() << "[Stockfish] Error string:" << process->errorString();
        return false;
    }

    // BUOC 2: Khoi tao UCI protocol
    sendCommand("uci");

    // Doi Stockfish san sang
    if (!process->waitForReadyRead(3000)) {
        qDebug() << "[Stockfish] WARNING: No response from engine";
    }
    // Doc het output
    process->readAllStandardOutput();

    sendCommand("isready");
    sendCommand(QString("setoption name Skill Level value %1").arg(difficulty));
    sendCommand("ucinewgame");

    qDebug() << "[Stockfish] Engine started successfully! Difficulty:" << difficulty;
    return true;
}

void StockfishManager::stopEngine()
{
    if (process->state() == QProcess::Running) {
        sendCommand("quit");
        process->waitForFinished(2000);
        if (process->state() == QProcess::Running) {
            process->kill();
        }
    }
}

bool StockfishManager::isRunning()
{
    return process->state() == QProcess::Running;
}

void StockfishManager::setDifficulty(int level)
{
    difficulty = qBound(0, level, 20);
    if (isRunning()) {
        sendCommand(QString("setoption name Skill Level value %1").arg(difficulty));
    }
    qDebug() << "[Stockfish] Difficulty set to:" << difficulty;
}

void StockfishManager::getBestMove(const QString &fen, int thinkTimeMs)
{
    if (!isRunning()) {
        qDebug() << "[Stockfish] ERROR: Engine not running! Trying to restart...";
        if (!startEngine()) {
            qDebug() << "[Stockfish] FATAL: Cannot restart engine!";
            return;
        }
    }

    qDebug() << "[Stockfish] Requesting move with FEN:" << fen;
    qDebug() << "[Stockfish] Think time:" << thinkTimeMs << "ms";

    sendCommand("position fen " + fen);
    sendCommand(QString("go movetime %1").arg(thinkTimeMs));
}

void StockfishManager::sendCommand(const QString &cmd)
{
    if (process->state() == QProcess::Running) {
        process->write((cmd + "\n").toUtf8());
        qDebug() << "[Stockfish] <<" << cmd;
    } else {
        qDebug() << "[Stockfish] SKIP (not running):" << cmd;
    }
}

void StockfishManager::onDataReceived()
{
    while (process->canReadLine()) {
        QString line = process->readLine().trimmed();

        if (line.isEmpty()) continue;

        qDebug() << "[Stockfish] >>" << line;

        if (line.startsWith("bestmove")) {
            QStringList parts = line.split(" ");
            if (parts.size() >= 2) {
                QString move = parts[1];
                if (move == "(none)") {
                    qDebug() << "[Stockfish] No valid move (game over?)";
                    return;
                }
                qDebug() << "[Stockfish] Best move found:" << move;
                emit bestMoveReady(move);
            }
        }
    }
}
