CREATE TABLE IF NOT EXISTS Users (Id INTEGER PRIMARY KEY, Username TEXT, Password TEXT);
CREATE TABLE IF NOT EXISTS ThirdPartyResources (Name TEXT PRIMARY KEY, Url TEXT);
INSERT OR IGNORE INTO ThirdPartyResources (Name, Url) VALUES ('yq', 'https://github.com/mikefarah/yq/releases/download/v4.53.3/yq_linux_amd64');
