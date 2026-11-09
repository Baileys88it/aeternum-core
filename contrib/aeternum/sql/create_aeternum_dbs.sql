-- Aeternum isolated databases (no passwords in this file).
-- Run against the MySQL instance that serves Aeternum (port 3308).
CREATE DATABASE IF NOT EXISTS auth CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE IF NOT EXISTS characters CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE IF NOT EXISTS world CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE IF NOT EXISTS hotfixes CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- Grant a local app user you create yourself, for example:
--   CREATE USER IF NOT EXISTS 'trinity'@'localhost' IDENTIFIED BY 'choose-locally';
--   CREATE USER IF NOT EXISTS 'trinity'@'127.0.0.1' IDENTIFIED BY 'choose-locally';
-- Then:
GRANT ALL PRIVILEGES ON auth.* TO 'trinity'@'localhost';
GRANT ALL PRIVILEGES ON auth.* TO 'trinity'@'127.0.0.1';
GRANT ALL PRIVILEGES ON characters.* TO 'trinity'@'localhost';
GRANT ALL PRIVILEGES ON characters.* TO 'trinity'@'127.0.0.1';
GRANT ALL PRIVILEGES ON world.* TO 'trinity'@'localhost';
GRANT ALL PRIVILEGES ON world.* TO 'trinity'@'127.0.0.1';
GRANT ALL PRIVILEGES ON hotfixes.* TO 'trinity'@'localhost';
GRANT ALL PRIVILEGES ON hotfixes.* TO 'trinity'@'127.0.0.1';
FLUSH PRIVILEGES;
