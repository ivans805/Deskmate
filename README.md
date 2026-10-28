# Deskmate
A simple desk gadget that communicates over serial and shows your currently playing media and lets you control playback like prev, next, play, and pause.
<img width="4288" height="3216" alt="DSCF1769" src="https://github.com/user-attachments/assets/ed995456-e218-4003-9708-965843ef4cf7" />
<img width="4288" height="3216" alt="DSCF1766" src="https://github.com/user-attachments/assets/909c1727-2af1-417c-ba66-127539a969f2" />
# How to set up
## Material list
- 1x ESP32-2432S028R Board (Cheap yellow display) With or without touch
  <img width="351" height="351" alt="image" src="https://github.com/user-attachments/assets/44020a5d-d96c-4b3f-8328-b4feb59730d4" />
- 1x Bare rotary encoder module with nut and washer for mounting
  <img width="1600" height="1200" alt="image" src="https://github.com/user-attachments/assets/f7e434db-39a6-4d30-98c1-5c9b49dc1091" />
- Wires for soldering
- 3D printed case
- 1x 4 pin Micro JST 1,25mm connector
  <img width="160" height="89" alt="image" src="https://github.com/user-attachments/assets/358e340f-90f4-4c29-bb2d-4ed77883ff6d" />
  
<img width="4624" height="3468" alt="IMG20261028213127" src="https://github.com/user-attachments/assets/b6f77010-73d4-487c-a0e9-b7b39f510506" />

## Assembly

Firstly solder the encoder as shown in the image. Specificly you solder like that: 
- encoder GND: 4th pin of the JST connector (the one that is connected to GND when plugged in)
- encoder A: 2nd pin on the JST connector (The one that connects to GPIO 22 when plugged in)
- encoder B: 3rd pin on the JST connector (The one that connects to GPIO 27 when plugged in)
  <img width="956" height="608" alt="image" src="https://github.com/user-attachments/assets/5f09d6c5-b60c-4286-ad48-8ac44c8cd194" />
- encoder Button: GPIO 4 on the esp32 WROOM module (as shown in the image)
  <img width="3468" height="4624" alt="IMG20261028213053" src="https://github.com/user-attachments/assets/5d268ffe-4d3f-4e91-84e4-b4420a089b5d" />

Then you can move on and assemble the case like this:

1. cut away the plastic on the connector part
<img width="3468" height="4624" alt="IMG20261028213253" src="https://github.com/user-attachments/assets/aae03ac5-d8c4-487c-b989-503df374ce4c" />

2. allign the display with the pegs in the case (make sure to be carefull so that the solder joints dont break!)
   <img width="3468" height="4624" alt="IMG20261028213317" src="https://github.com/user-attachments/assets/f848b7a7-d870-483e-9565-62438c1fac51" /> <img width="3468" height="4624" alt="IMG20261028213312" src="https://github.com/user-attachments/assets/70977b09-c89f-46ce-b020-c363483376ab" />

Make sure it goes in like this
<img width="3468" height="4624" alt="IMG20261028213329" src="https://github.com/user-attachments/assets/dbad2864-d44a-4e15-b6f7-158d5552798c" />

3. Connect the JST connector and mount the rotary encoder
   <img width="4624" height="3468" alt="IMG20261028213515" src="https://github.com/user-attachments/assets/89aae355-5b1e-46e6-8d42-f357d5ba77d9" />
   <img width="4624" height="3468" alt="IMG20261028213611" src="https://github.com/user-attachments/assets/fcdc7041-eb99-4228-816e-23f64c85f882" />

So far the build should look like this
<img width="4624" height="3468" alt="IMG20261028213645" src="https://github.com/user-attachments/assets/f8368f7b-99dd-468b-a8f5-38801571369e" />

4. Manage the cables as shown in the image (make a cut in the hole next to the rotary encoder mount)
<img width="3468" height="4624" alt="IMG20261028213721" src="https://github.com/user-attachments/assets/75473a50-4749-4ce6-b979-cf5ae9d4eaa5" />
<img width="3468" height="4624" alt="IMG20261028213739" src="https://github.com/user-attachments/assets/ea639502-39ba-4050-9844-09ad32d2bed0" />

okay the assembly is done!

## Flash and connect
In this repo i included a DeskmateOS Lite folder. Inside it theres a .ino sketch you can open it and here is how you can flash your deskmate device. For this you can use a tutorial on hw to set up lvgl with the cyd: https://randomnerdtutorials.com/esp32-cyd-lvgl-display-image/#more-161686

# Finnishing up
The libraries you need are LVGL, LVGL_CYD and TFT_eSPI.
Sadly the desktop app for Deskmate only exists fo Windows so far.
Lastly run the Deskmate connect.exe app and also put it in shell:startup if you want by pressing win + r, typing shell:startup and putting the .exe file there.

# Support
If you have any issues or quetions, then create an issue under this repo or start a discussion.
You can also contact me on discord as @def.not1v4n.



