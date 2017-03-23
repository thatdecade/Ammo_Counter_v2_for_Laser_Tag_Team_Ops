# Ammo Counter for Laser Tag Team Ops LTAR Tagger
This is my second attempt to make an Ammo Counter for the LaserTag LTAR Tagger.  The v1 worked fine, but could get out of sync from missed tags.  This version fixes the sync issues by counting the beacon LED pulses instead of the IR tags.

Demo Video:               
https://www.youtube.com/watch?v=Hjc7z6LJstw

Assembly Notes:       
http://www.westaby.net/2013/12/ltar-display/

**Usage:**

Unhosted Games:
1. Turn tagger off and on, this will wake the display
2. Press shield button to switch between 10 and 25 health gametypes.
3. Press trigger to start game, 10 second countdown begins
4. Display will show remaining health / shields during game.

Hosted Games:
1. Setup hosted game with the health / shield numbers using instructions for number entry.
2. Turn the tagger off and on while holding the shield button, display will show dashes '--' until you release the shield button to confirm advanced mode.
3. Join hosted game by holding reload (dome light is blinking).
4. Once you have joined (dome light stops blinking and is solid lit), press the shield button twice to tell the display the game is ready.
5. The display will wait for the dome light to go dark, indicating the hosted game has started, a 10 second countdown begins. :)

Number Entry:
1. Turn on tagger while holding the shield button, this wakes the display and enters advanced mode.  The display will show dashes '--' to indicate selection.
2. Let go of shield button and the health number is displayed first.
3. To change the number, pull the trigger.  You may also hold the trigger down for faster number entry.
4. Press the shield button to save the health number.
5. The shield number is now displayed second.
6. You may also change this number.
7. Press the shield button a second time to save the shield number.
8. The game will begin after a 10 second countdown begins.

Note: The circuit has a memory, the last entered health / shield numbers will always be displayed when advanced mode is selected.
