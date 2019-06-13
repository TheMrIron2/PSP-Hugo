	Welcome to PCE Engine Emulator

Original Authors of Hu-go

  See AUTHORS.txt file

Author of the PSP port version 

  Ludovic Jacomme alias Zx-81 zx81.zx81(at)gmail.com

  Homepage: http://zx81.zx81.free.fr


1. INTRODUCTION
   ------------

  Hu-Go emulates the NEC PC Engine console on many system such as
  Linux and Windows. (see http://www.zeograd.com/ for details)

  PSP-HUGO is a port on PSP of latest HUGO version.

  This package is under GPL Copyright, read COPYING file for
  more information about it.

  Special thanks to my friend Nicolas C. for his support and kindness, 
  without him i would have never found enough motivation to port 
  Hu-Go to PSP.

  This version of PSP-Hugo supports IRDA-Joystick box designed by
  my good friend Buzz ( see http://buzz.computer.free.fr ). 
  Schematic and PIC source code is provided in contrib folder.


2. INSTALLATION
   ------------

  Unzip the zip file, and copy the content of the directory fw5x or fw15
  (depending of the version of your firmware) on the psp/game, psp/game150,
  or psp/game5XX if you use custom firmware 5.xx-M33.

  It has been developped on linux for Firmware 5.01-m33 and i hope it works
  also for other firmwares.

  For any comments or questions on this version, please visit 
  http://zx81.zx81.free.fr or http://zx81.dcemu.co.uk


3. CONTROL
   ------------

3.1 - Virtual keyboard

  In the PC-Engine emulator window, there are three different mapping 
  (standard, left trigger, and right Trigger mappings). 
  You can toggle between while playing inside the emulator using 
  the two PSP trigger keys.

    -------------------------------------
    PSP        PC-Engine          (standard)
  
    Square     Select
    Triangle   Run 
    Circle     A  
    Cross      B
    Left       Left
    Down       Down
    Right      Right
    Up         Up

    Analog     Joystick

    -------------------------------------
    PSP        PC-Engine (left trigger)
  
    Square     FPS  
    Triangle   LOAD Snapshot
    Circle     Swap digital / Analog
    Cross      SAVE Snapshot
    Up         Inc delta Y
    Down       Dec delta Y
    Left       Render mode
    Right      Render mode

    -------------------------------------
    PSP        PC-Engine (right trigger)
  
    Square     Select
    Triangle   Run
    Circle     A
    Cross      Auto-fire
    Up         Up
    Down       Down
    Left       Dec fire
    Right      Inc fire
  
    Analog     Joystick
    
    Press Start  + L + R   to exit and return to eloader.
    Press Select           to enter in emulator main menu.
    Press Start            open/close the On-Screen keyboard

  In the main menu

    RTrigger   Reset the emulator

    Triangle   Go Up directory
    Cross      Valid
    Circle     Valid
    Square     Go Back to the emulator window

  The On-Screen Keyboard of "Danzel" and "Jeff Chen"

    Use Analog stick to choose one of the 9 squares, and
    use Triangle, Square, Cross and Circle to choose one
    of the 4 letters of the highlighted square.

    Use LTrigger and RTrigger to see other 9 squares 
    figures.

3.2 - IR keyboard

  You can also use IR keyboard. Edit the pspirkeyb.ini file
  to specify your IR keyboard model, and modify eventually
  layout keyboard files in the keymap directory.

  The following mapping is done :

  IR-keyboard   PSP

  Cursor        Digital Pad

  Tab           Start
  Ctrl-W        Start

  Escape        Select
  Ctrl-Q        Select

  Ctrl-E        Triangle
  Ctrl-X        Cross
  Ctrl-S        Square
  Ctrl-F        Circle
  Ctrl-Z        L-trigger
  Ctrl-C        R-trigger

  In the emulator window you can use the IR keyboard to
  enter letters, special characters and digits.

4. LOADING ROM FILES (.pce)
   -----------------------

  If you want to load rom image in your emulator, you have to put 
  your rom file (with .pce file extension) on your PSP
  memory stick in the 'roms' directory. 

  Then, while inside emulator, just press SELECT to enter in the emulator 
  main menu, choose "Load Rom", and then using the file selector choose one 
  rom image  file to load in your emulator.

  You can use the virtual keyboard in the file requester menu to choose the
  first letter of the game you search (it might be useful when you have tons of
  games in the same folder). Entering several time the same letter let you
  choose sequentially files beginning with the given letter. You can use the
  Run key of the virtual keyboard to launch the rom.

5. LOADING ISO FILES (.ISO)
   -----------------------

  If you want to load iso image in your emulator, you have to put your file
  (with .iso file extension) on your PSP memory stick in the 'cd-roms'
  directory. 

6. LOADING HCD FILES (.HCD)
   -----------------------

HCD format describes CD tacks for Hu-go. Here is an example of that 
file format:

[main]
first_track=1
last_track=22
minimum_bios=1

[track1]
type=AUDIO
filename=Track01.mp3
begin=lsn,0

[track2]
type=CODE
filename=Track02.iso
begin=lsn,3890

....

See Hu-go web site for more details.

If you want to load a hcd file with all track datas in your emulator, you have
to put your file (with .hcd file extension) on your PSP memory stick in a
sub-folder of the 'cd-roms' directory. 

For example for Dracula X i have the following folders :

cd-roms
`-- dracx
    |-- Track01.mp3
    |-- Track02.iso
    |-- ...
    |-- Track22.iso
    `-- dracx.hcd

An example for dracx.hcd file is present in doc folder.

Audio tracks are supported only in MP3 format with
stereo 16 bits samples at 22k or 44k rate.

7. LOADING TOC FILES (.TOC)
   -----------------------

TOC format describes CD tacks. Here is an example of that 
file format:

Track 01 Audio 00:02:00 LBA=000000
Track 02 Data  00:49:65 LBA=003590
Track 03 Audio 01:27:23 LBA=006398
...

If you want to load a TOC file with all track datas in your emulator, you have
to put your file (with .toc file extension) on your PSP memory stick in a
sub-folder of the 'cd-roms' directory. 

For example for R-Type i have the following folders :

cd-roms
`-- r-type
    |-- 01.mp3
    |-- 02.iso
    |--...
    |-- 46.iso
    `-- rtype.toc

An example for rtype.toc file is present in doc folder.

Audio tracks are supported only in MP3 format with
stereo 16 bits samples at 22k or 44k rate.

8. HOW TO CREATE TOC FILE FROM CUE/BIN
   -----------------

You can use TurboRipV100 to create CD rom files compatible with PSP-Hugo.
Mount your CUE/BIN file using virtual CD tool such as Daemon tools, then
run TurboRipV100 with the following arguments :

   TurboRip.exe /PCEP /RS=22050 

After selecting your virtual CD-ROM id, TurboRip will ripp all audio
and data tracks in a folder and it will generate a toc file compatible 
with PSP-Hugo.

Copy the generated folder in the psphugo/cd-roms folder on your psp.
Using the CD-Rom menu of the emulator you will then be able to enjoy your
game.

9. CHEAT CODE (.CHT)
   -----------------

You can use cheat codes with PSP-Hugo. 
You can add your own cheat codes in the global cheat.txt file and then import 
them in the cheat menu.

The cheat.txt file provided with PSP-Hugo is given as an example, and i
haven't tested all cheat codes. I am pretty sure that most of them won't work.
But see below how to find and create you own cheat code using PSP-Hugo cheat
engine.

All cheat codes you have specified for a game can be save in a CHT file in
'cht' folder.
Those cheat codes would then be automatically loaded when you start the game.

The CHT file format is the following :
#
# Enable, Address, Value, Comment
#
1,36f,3,Cheat comment

Using the Cheat menu you can search for modified bytes in RAM between current
time and the last time you saved the RAM. It might be very usefull to find
"poke" address by yourself, monitoring for example life numbers.

To find a new "poke address" you can proceed as follow :

Let's say you're playing PC kid and you want to find the memory address where
"number lives" is stored.

. Start a new game in PC kid
. Enter in the cheat menu. 
. Choose Save Ram to save initial state of the memory. 
. Specify the number of lives you want to find in "Scan Old Value" field.
  (for PC kid the initial lives number is 2)
. Go back to the game and loose a life.
. Enter in the cheat menu. 
. Specify the number of lives you want to find in "Scan New Value" field.
  (for PC kid the lives number is now 1)
. In Add Cheat you have now one matching Address
  (for PC kid it's 0DAF)
. Specify the Poke value you want (for example 3) and add a new cheat with
  this address / value.

The cheat is now activated in the cheat list and you can save it using the
"Save cheat" menu.

Let's now enjoy PC kid with infinite life !!


10. LOADING KEY MAPPING FILES
   -------------------------

  For given games, the default keyboard mapping between PSP Keys and PC Engine
  keys, is not suitable, and the game can't be played on PSPHUGO.

  To overcome the issue, you can write your own mapping file. Using notepad for
  example you can edit a file with the .kbd extension and put it in the kbd 
  directory.

  For the exact syntax of those mapping files, have a look on sample files already
  presents in the kbd directory (default.kbd etc ...).

  After writting such keyboard mapping file, you can load them using 
  the main menu inside the emulator.

  If the keyboard filename is the same as the rom file then when you load 
  this file, the corresponding keyboard file is automatically loaded !

  You can now use the Keyboard menu and edit, load and save your
  keyboard mapping files inside the emulator. The Save option save the .kbd
  file in the kbd directory using the "Game Name" as filename. The game name
  is displayed on the right corner in the emulator menu.

  If you have saved the state of a game, then a thumbnail image will be
  displayed in the file requester while selecting any file (roms, keyboard,
  settings) with game name, to help you to recognize that game later.

11. COMMENTS
   ------------

  You can write your own comments for games using the "Comment" menu.
  The first line of your comments would then be displayed in the file
  requester menu while selecting the given file name 
  (roms, keyboard, settings)

  
12. SETTINGS
   ------------

  You can modify several settings value in the settings menu of this emulator.
  The following parameters are available :

  Sound enable       : enable or disable the sound
  Sound volume boost : factor to apply to the volume, useful to 
                       increase the sound volume on given game.
  Sound frequency    : sound quality, it could be 22k/44k mono/stereo
  IRDA device        : Type of IRDA device connected to PSP
                       It could be None, Keyboard (such as Targus) 
                       or Joystick (see IRDA Joy section)
  CD audio track     : enable CD audio track support
  Speed limiter      : limit the speed to a given fps value
  Skip frame         : to skip frame and increase emulator speed
  Overclock          : useful to increase significantly emulator speed
                       but you may encounter graphical glitches
  Display fps        : display real time fps value 
  Render mode        : many render modes are available with different 
                       geometry that should covered all games requirements
  Delta Y            : move the center of the screen vertically
  Vsync              : wait for vertical signal between each frame displayed
  Clock frequency    : PSP clock frequency
  CD Clock frequency : PSP clock frequency used when a MP3/CD audio track is played


12. JOYSTICK SETTINGS
   ------------

  You can modify several joystick settings value in the settings menu of this emulator.
  The following parameters are available :

  Active Joystick    : joystick player, it could be 1 or 2
  Swap Analog/Cursor : swap key mapping between PSP analog pad and PSP digital pad
  Auto fire period   : auto fire period
  Auto fire mode     : auto fire mode active or not

  See IRDA-Joy section for other parameters description.


13. IRDA-JOY SETTINGS
   ------------

  This version of PSP-Hugo supports IRDA-Joystick box
  ( see http://buzz.computer.free.fr for all details ). 

  IRDA mode          : type of DB9 device connected to the "Irda Joystick box". It could be None, 
                       Joystick, Single or double paddle.
  IRDA debug         : enable or disable debug mode to display data sent by the "Irda Joystick box".

  You can then define Irda Joystick box keys mapping to PSP keys. The default
  mapping is the following : 

  Joy Up             : Digital Up
  Joy Down           : Digital Down 
  Joy Left           : Digital Left
  Joy Right          : Digital Right
  Joy Fire           : Cross
  Paddle 1 +         : Analog Right
  Paddle 1 -         : Analog Left
  Paddle 1 Fire      : Cross
  Paddle 2 +         : Analog Up
  Paddle 2 -         : Analog Down
  Paddle 2 Fire      : Circle


14. PERFORMANCES
   ------------

By default the PSP clock frequency is set to 266Mhz and it should be enough
for most of all games. 

If you run CD-rom games with audio tracks, PSP-Hugo will automatically
increase the clock frequency while playing MP3 tracks to 300 Mhz.
(You may modify this parameter in the settings menu).

Some games such as PC-kid are fullspeed at 222Mhz with overclock parameter
set to 32 and using 22Khz mono sound. If you want to save your battery
and play longuer it might be a good choice.

CD-Rom games are faster if you disable "CD audio track" support, so if you
encounter speed issue it might be a solution.

If you encounter graphical glitches then you may set Overclock value to 0 
and increase the PSP clock frequency for a better emulation experience.
  
15. COMPILATION
   ------------

  It has been developped under Linux using gcc with PSPSDK (gcc v4.3.1)
  To rebuild the homebrew run the Makefile in the src archive.

