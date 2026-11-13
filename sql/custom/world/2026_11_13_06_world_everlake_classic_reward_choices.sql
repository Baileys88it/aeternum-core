-- Data from Efficient Games (https://efficient.games), CC BY 4.0. Client build 1.60.1.70862.
START TRANSACTION;

UPDATE quest_template
SET RewardItem1 = 0,
    RewardAmount1 = 0,
    RewardChoiceItemID1 = 1893,
    RewardChoiceItemQuantity1 = 1,
    RewardChoiceItemID2 = 270012,
    RewardChoiceItemQuantity2 = 1,
    RewardChoiceItemID3 = 270013,
    RewardChoiceItemQuantity3 = 1
WHERE ID = 167 AND LogTitle = 'Oh Brother. . .'
  AND RewardItem1 = 1893 AND RewardAmount1 = 1 AND RewardChoiceItemID1 = 0 AND RewardChoiceItemQuantity1 = 0 AND RewardChoiceItemDisplayID1 = 0
  AND RewardItem2 = 0 AND RewardAmount2 = 0 AND RewardChoiceItemID2 = 0 AND RewardChoiceItemQuantity2 = 0 AND RewardChoiceItemDisplayID2 = 0
  AND RewardItem3 = 0 AND RewardAmount3 = 0 AND RewardChoiceItemID3 = 0 AND RewardChoiceItemQuantity3 = 0 AND RewardChoiceItemDisplayID3 = 0
  AND RewardItem4 = 0 AND RewardAmount4 = 0 AND RewardChoiceItemID4 = 0 AND RewardChoiceItemQuantity4 = 0 AND RewardChoiceItemDisplayID4 = 0
  AND RewardChoiceItemID5 = 0 AND RewardChoiceItemQuantity5 = 0 AND RewardChoiceItemDisplayID5 = 0
  AND RewardChoiceItemID6 = 0 AND RewardChoiceItemQuantity6 = 0 AND RewardChoiceItemDisplayID6 = 0;

UPDATE quest_template
SET RewardChoiceItemID4 = 270000,
    RewardChoiceItemQuantity4 = 1
WHERE ID = 176 AND LogTitle = 'Wanted:  "Hogger"'
  AND RewardItem1 = 0 AND RewardAmount1 = 0 AND RewardChoiceItemID1 = 6085 AND RewardChoiceItemQuantity1 = 1 AND RewardChoiceItemDisplayID1 = 0
  AND RewardItem2 = 0 AND RewardAmount2 = 0 AND RewardChoiceItemID2 = 6084 AND RewardChoiceItemQuantity2 = 1 AND RewardChoiceItemDisplayID2 = 0
  AND RewardItem3 = 0 AND RewardAmount3 = 0 AND RewardChoiceItemID3 = 6215 AND RewardChoiceItemQuantity3 = 1 AND RewardChoiceItemDisplayID3 = 0
  AND RewardItem4 = 0 AND RewardAmount4 = 0 AND RewardChoiceItemID4 = 0 AND RewardChoiceItemQuantity4 = 0 AND RewardChoiceItemDisplayID4 = 0
  AND RewardChoiceItemID5 = 0 AND RewardChoiceItemQuantity5 = 0 AND RewardChoiceItemDisplayID5 = 0
  AND RewardChoiceItemID6 = 0 AND RewardChoiceItemQuantity6 = 0 AND RewardChoiceItemDisplayID6 = 0;

UPDATE quest_template
SET RewardItem1 = 0,
    RewardAmount1 = 0,
    RewardChoiceItemID1 = 4744,
    RewardChoiceItemQuantity1 = 1,
    RewardChoiceItemID2 = 270067,
    RewardChoiceItemQuantity2 = 1,
    RewardChoiceItemID3 = 270068,
    RewardChoiceItemQuantity3 = 1
WHERE ID = 684 AND LogTitle = 'Wanted!  Marez Cowl'
  AND RewardItem1 = 4744 AND RewardAmount1 = 1 AND RewardChoiceItemID1 = 0 AND RewardChoiceItemQuantity1 = 0 AND RewardChoiceItemDisplayID1 = 0
  AND RewardItem2 = 0 AND RewardAmount2 = 0 AND RewardChoiceItemID2 = 0 AND RewardChoiceItemQuantity2 = 0 AND RewardChoiceItemDisplayID2 = 0
  AND RewardItem3 = 0 AND RewardAmount3 = 0 AND RewardChoiceItemID3 = 0 AND RewardChoiceItemQuantity3 = 0 AND RewardChoiceItemDisplayID3 = 0
  AND RewardItem4 = 0 AND RewardAmount4 = 0 AND RewardChoiceItemID4 = 0 AND RewardChoiceItemQuantity4 = 0 AND RewardChoiceItemDisplayID4 = 0
  AND RewardChoiceItemID5 = 0 AND RewardChoiceItemQuantity5 = 0 AND RewardChoiceItemDisplayID5 = 0
  AND RewardChoiceItemID6 = 0 AND RewardChoiceItemQuantity6 = 0 AND RewardChoiceItemDisplayID6 = 0;

UPDATE quest_template
SET RewardChoiceItemID3 = 270080,
    RewardChoiceItemQuantity3 = 1
WHERE ID = 685 AND LogTitle = 'Wanted!  Otto and Falconcrest'
  AND RewardItem1 = 0 AND RewardAmount1 = 0 AND RewardChoiceItemID1 = 5247 AND RewardChoiceItemQuantity1 = 1 AND RewardChoiceItemDisplayID1 = 0
  AND RewardItem2 = 0 AND RewardAmount2 = 0 AND RewardChoiceItemID2 = 4745 AND RewardChoiceItemQuantity2 = 1 AND RewardChoiceItemDisplayID2 = 0
  AND RewardItem3 = 0 AND RewardAmount3 = 0 AND RewardChoiceItemID3 = 0 AND RewardChoiceItemQuantity3 = 0 AND RewardChoiceItemDisplayID3 = 0
  AND RewardItem4 = 0 AND RewardAmount4 = 0 AND RewardChoiceItemID4 = 0 AND RewardChoiceItemQuantity4 = 0 AND RewardChoiceItemDisplayID4 = 0
  AND RewardChoiceItemID5 = 0 AND RewardChoiceItemQuantity5 = 0 AND RewardChoiceItemDisplayID5 = 0
  AND RewardChoiceItemID6 = 0 AND RewardChoiceItemQuantity6 = 0 AND RewardChoiceItemDisplayID6 = 0;

COMMIT;
