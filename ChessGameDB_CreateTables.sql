CREATE DATABASE chessgameDB;
GO

USE	chessgameDB;
GO

-- player
CREATE TABLE Players (
	PlayerID INT PRIMARY KEY IDENTITY(1,1),
	PlayerName NVARCHAR(100) NOT NULL,
	CreatedDate DATETIME DEFAULT GETDATE()
);

-- match
CREATE TABLE Games (
	GameID INT PRIMARY KEY IDENTITY(1,1),
	WhitePlayerID INT,
	BlackPlayerID INT,
	Result NVARCHAR(20),
	StartTime DATETIME DEFAULT GETDATE(),
	EndTime DATETIME NULL,
	FOREIGN KEY (WhitePlayerID) REFERENCES Players(PlayerID),
	FOREIGN KEY (BlackPlayerID) REFERENCES Players(PlayerID),
);

-- moves
CREATE TABLE Moves (
	MoveID INT PRIMARY KEY IDENTITY(1,1),
	GameID INT,
	MoveNumber INT,
	MoveNotation NVARCHAR(10),
	Team NVARCHAR(10),
	MoveTime DATETIME DEFAULT GETDATE(),
	FOREIGN KEY (GameID) REFERENCES Games(GameID)
);

CREATE TABLE PlayerStats(
	StatID INT PRIMARY KEY IDENTITY(1,1),
	PlayerID INT,
	Wins INT DEFAULT 0,
	Losses INT DEFAULT 0,
	Draws INT DEFAULT 0,
	FOREIGN KEY (PlayerID) REFERENCES Players(PlayerID)
);

PRINT 'Database ChessGameDB created successfully!';