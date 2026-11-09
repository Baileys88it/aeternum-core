-- Aeternum arena (2026-10-28): Mak'gora Arena start positions checked with navcheck/vmcheck.
-- Navmesh: floor at z 1453.41 / 1453.52, same component (21571), direct 60y path between the two starts, no ceiling/wall.
-- Z was 1455.3 (1.9y above the floor): lowered to 0.5y above the navmesh. Active after the next restart.
UPDATE `world_safe_locs` SET `LocZ`=1453.9, `Comment`='Aeternum arena - Mak''gora Arena - start gold (navcheck ok)' WHERE `ID`=1105000;
UPDATE `world_safe_locs` SET `LocZ`=1454.0, `Comment`='Aeternum arena - Mak''gora Arena - start green (navcheck ok)' WHERE `ID`=1105001;
