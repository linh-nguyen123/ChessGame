USE ChessGameDB;

-- player
SELECT * FROM Players;

-- match
SELECT 
    g.GameID,
    p1.PlayerName AS WhitePlayer,
    p2.PlayerName AS BlackPlayer,
    g.Result,
    g.StartTime,
    g.EndTime
FROM Games g
JOIN Players p1 ON g.WhitePlayerID = p1.PlayerID
JOIN Players p2 ON g.BlackPlayerID = p2.PlayerID;

-- all moves
SELECT 
    m.MoveID,
    m.GameID,
    m.MoveNumber,
    m.Team,
    m.MoveNotation,
    m.MoveTime
FROM Moves m
ORDER BY m.GameID, m.MoveNumber;

-- analytic players
SELECT 
    p.PlayerName,
    ps.Wins,
    ps.Losses,
    ps.Draws
FROM PlayerStats ps
JOIN Players p ON ps.PlayerID = p.PlayerID;