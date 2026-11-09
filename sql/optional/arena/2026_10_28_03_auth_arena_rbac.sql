-- Aeternum arena (2026-10-28): RBAC permissions for the custom .arena commands (RBAC.h: RBAC_PERM_COMMAND_WF_ARENA*).
-- Apply to: auth.
-- 1100: .arena queue/leave/rating/top  -> linked to 199 (Role: Player Commands, all players)
-- 1101: .arena season end               -> linked to 196 (Role: Administrator Commands)
DELETE FROM `rbac_linked_permissions` WHERE `linkedId` IN (1100,1101);
DELETE FROM `rbac_permissions` WHERE `id` IN (1100,1101);
INSERT INTO `rbac_permissions` (`id`,`name`) VALUES
(1100,'Command: arena queue/leave/rating/top (Aeternum)'),
(1101,'Command: arena season end (Aeternum)');
INSERT INTO `rbac_linked_permissions` (`id`,`linkedId`) VALUES
(199,1100),
(196,1101);
