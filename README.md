# warehouse-automation-system
Smart robotic arm warehouse automation with NFC-based sorting

1. Introduction
This manual provides a complete guide for setting up, connecting, and operating the Smart Warehouse Automation System prototype. It is written for a reader who has not previously interacted with the system. By following this document from start to finish, you will be able to power on the prototype, connect it to a Wi-Fi network, launch the backend server, start the web dashboard, and understand how the system operates during a normal sorting cycle.

1.1 What the System Does
The system autonomously sorts packages placed on a belt conveyor. When a package arrives at the scanning zone, the IR sensor stops the conveyor, and the NFC reader identifies the package. The ESP32 microcontroller then directs the robotic arm to pick the package and place it on the correct shelf and updates the inventory database over Wi-Fi. The web dashboard running on a laptop or PC displays real-time inventory status.

4.  Configuring the ESP32 Firmware
4.1 What You Need
•	A PC or laptop with the Arduino IDE installed (version 2.x recommended — download free from arduino.cc)
•	The ESP32 board support package installed in Arduino IDE (see Section 4.2)
•	A USB-A to Micro-USB cable to connect the ESP32 to your PC
•	The firmware source file: warehouse_arm.ino (included below)

4.2 Installing ESP32 Board Support in Arduino IDE (First-Time Only)
Step 1.	Open Arduino IDE.
Step 2.	Click File → Preferences.
Step 3.	In the "Additional boards manager URLs" field, paste the following URL and click OK:
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
Step 4.	Click Tools → Board → Boards Manager.
Step 5.	Search for "esp32" and install the package by Espressif Systems.
Step 6.	Once installed, click Tools → Board → ESP32 Arduino → "ESP32 Dev Module" to select the correct board.

4.3 Installing Required Libraries
The firmware depends on the following libraries. Install each one through Arduino IDE → Sketch → Include Library → Manage Libraries:

Table 4.1: Required Arduino Libraries
Library Name	Author / Notes
AccelStepper	Mike McCauley — search "AccelStepper"
ESP32Servo	Kevin Harrington — search "ESP32Servo"
Adafruit PN532	Adafruit — search "Adafruit PN532"
Adafruit BusIO	Adafruit — installed automatically with PN532
WiFi	Built-in to ESP32 board package — no install needed
HTTPClient	Built-in to ESP32 board package — no install needed

4.4 Changing the Wi-Fi Network Name and Password
The ESP32 connects to a Wi-Fi network to send inventory data to the backend server. You must update the firmware with the name (SSID) and password of the Wi-Fi network your laptop/PC is connected to.

Step 7.	Open the firmware file warehouse_arm.ino in Arduino IDE.
Step 8.	Locate the following two lines near the top of the file (around line 52):
const char* ssid     = "Aero";
const char* password = "AeroKing2026";
Step 9.	Replace "Aero" with the exact name of your Wi-Fi network and "AeroKing2026" with its password. Ensure you keep the quotation marks.
const char* ssid     = "YourNetworkName";
const char* password = "YourPassword";
Step 10.	Also update the server IP address. Locate this line:
http.begin("http://192.168.100.55:3000/api/package");
Step 11.	Replace 192.168.100.55 with the IP address of the laptop/PC running the backend server. To find your PC's IP address on Windows: open Command Prompt and type ipconfig. Look for the IPv4 Address under your Wi-Fi adapter. On Mac/Linux: open Terminal and type ifconfig or ip addr.
// Example — replace with your actual PC IP address:
http.begin("http://192.168.1.45:3000/api/package");

⚠   WARNING
The ESP32 and the PC running the server must be connected to the same Wi-Fi network. If they are on different networks, the ESP32 will not be able to send data to the server.

4.5 Uploading the Firmware to the ESP32
Step 12.	Connect the ESP32 to your PC using the Micro-USB cable.
Step 13.	In Arduino IDE, click Tools → Port and select the COM port that appeared when you plugged in the ESP32 (e.g. COM3 on Windows or /dev/ttyUSB0 on Linux/Mac).
Step 14.	Click the Upload button (right-facing arrow ▶) or press Ctrl + U.
Step 15.	Wait for the upload to complete. The IDE will display "Done uploading" at the bottom.
Step 16.	Disconnect the USB cable. The ESP32 is now ready to run on 5 V from the buck converter.

ℹ   NOTE
When uploading the code to the ESP32, hold down the BOOT button on the ESP32 when "Connecting..." appears, and release the moment the upload process begins as shown in the output monitor.

4.6 Adding a New NFC Tag (Adding a New Package Type)
The firmware maps each NFC tag's unique ID (UID) to a product name, shelf location, and shelf step position. To add a new tag, you need to know its UID and then add an entry to the checkNFC() function.

Step A — Find the UID of a New Tag
Step 17.	Open the Serial Monitor in Arduino IDE (Tools → Serial Monitor). Set baud rate to 115200.
Step 18.	Hold the new NFC tag over the PN532 reader. The ESP32 will print the tag's UID to the Serial Monitor, for example: UID: 5393A7E7520001
Step 19.	Copy this UID string.

Step B — Register the Tag in the Firmware
Step 20.	Open warehouse_arm.ino in Arduino IDE.
Step 21.	Find the block of if / else if statements inside the checkNFC() function that looks like this:
if (uidString == "5393A7E7520001") {
  product = "Coca-Cola"; location = "Shelf 1"; shelfPosition = 2000;
}
else if (uidString == "53D2B0E7520001") {
  product = "Fanta”; location = "Shelf 2"; shelfPosition = 8000;
}
Step 22.	Add a new else if block for the new tag. The shelfPosition value (in stepper steps) determines which shelf the arm moves to. Use the existing values as a guide — each shelf is separated by approximately 6000 steps:
else if (uidString == "PASTE_YOUR_NEW_UID_HERE") {
  product = "YourProductName";
  location = "Shelf 5";
  shelfPosition = 26000;   // adjust as needed
}
Step 23.	Re-upload the firmware following the steps in Section 4.5.

 
5.  Setting Up the Backend Server (Node.js)
5.1 Installing Node.js
Step 24.	Open a browser and go to https://nodejs.org
Step 25.	Click the LTS (Long-Term Support) download button. This downloads a .msi installer on Windows or a .pkg installer on Mac.
Step 26.	Run the installer and follow the on-screen prompts. Accept all default settings and ensure the option "Add to PATH" is ticked.
Step 27.	Once installation is complete, open Command Prompt (Windows) or Terminal (Mac/Linux).
Step 28.	Verify the installation by typing the following commands and pressing Enter after each. Both should print a version number (e.g. v20.11.0):
node --version
npm --version

5.2 Installing Server Dependencies
The server uses two Node.js packages: Express (web server) and Mongoose (MongoDB interface). Install them as follows:

Step 29.	Create a folder on your PC for the project, for example C:\WarehouseSystem or ~/WarehouseSystem.
Step 30.	Copy the file server.js into this folder.
Step 31.	Open Command Prompt / Terminal and navigate to the folder:
cd C:\WarehouseSystem
Step 32.	Initialise a Node.js project and install the required packages:
npm init -y
npm install express mongoose
Step 33.	Create a subfolder called public inside the project folder and place the web dashboard file (index.html) inside it:
WarehouseSystem/
  server.js
  public/
    index.html

5.3 Starting the Backend Server
Step 34.	Open Command Prompt / Terminal and navigate to the project folder.
Step 35.	Start the server:
node server.js
Step 36.	You should see the following output, confirming the server is running and MongoDB is connected:
MongoDB connected
Server running on port 3000

ℹ   NOTE
Keep this Command Prompt / Terminal window open the entire time the system is operating. Closing it will shut down the server and the ESP32 will no longer be able to send data.

✅  TIP
To start the server automatically every time without typing the command, you can use the Node.js process manager PM2. Install it with: npm install -g pm2 and start with: pm2 start server.js

 
6.  Installing and Setting Up MongoDB
6.1  Installing MongoDB Community Server
Step 37.	Open a browser and go to https://www.mongodb.com/try/download/community
Step 38.	Under "MongoDB Community Server", select your operating system from the dropdown and click Download.
Step 39.	Run the downloaded installer. On Windows, choose "Complete" installation when prompted.
Step 40.	On the "Service Configuration" screen, leave all settings at their defaults. Ensure the option "Install MongoDB as a Service" is ticked. This means MongoDB will start automatically every time your PC boots.
Step 41.	Complete the installation. You may optionally install MongoDB Compass (a visual database browser) which is useful for inspecting stored data.

6.2  Starting MongoDB Manually
If MongoDB is not configured to start automatically as a service, or if you need to start it manually, open Command Prompt as Administrator and run the following command:

& "C:\Program Files\MongoDB\Server\8.2\bin\mongod.exe" --dbpath "C:\mongodb\data\db"

ℹ   NOTE
You must first create the data folder if it does not already exist. Open Command Prompt and run: mkdir C:\mongodb\data\db

When MongoDB has started successfully, you will see a line in the output that includes:
Waiting for connections on port 27017

⚠   WARNING
Keep this Command Prompt window open while using the system. Closing it will shut down the database and the server will lose its connection.

6.3  Verifying MongoDB is Running
Step 42.	Open Command Prompt / Terminal.
Step 43.	Type the following and press Enter:
mongosh
Step 44.	If MongoDB is running, you will see a prompt like:
test>
Step 45.	Type exit and press Enter to close the MongoDB shell.

ℹ   NOTE
If mongosh is not recognised as a command, MongoDB Mongosh may need to be installed separately. Download it from https://www.mongodb.com/try/download/shell and add it to your system PATH.

6.4  How the Database Works
The database (named nfc_db) and its collection (packages) are created automatically the first time the server receives data from the ESP32. You do not need to create anything manually. Each time a package is scanned:

•	If the package UID has not been seen before, a new record is created storing its UID, product name, location, status (sorted), batch, and a timestamp.
•	If the package UID already exists in the database (i.e. the same package is scanned again), the existing record is updated with the new location and the current timestamp. This prevents duplicate entries.

ℹ   NOTE
To inspect the database contents directly, open MongoDB Compass, connect to mongodb://127.0.0.1:27017, and browse to nfc_db → packages.

 
7.  Web Dashboard Setup and Usage
7.1  Accessing the Dashboard
Step 46.	Ensure the backend server is running (Section 5.3).
Step 47.	Open any modern web browser (Google Chrome is recommended).
Step 48.	Type the following address in the address bar and press Enter:
http://localhost:3000
Step 49.	The dashboard should load immediately, showing the Overview page with live data.

ℹ   NOTE
If you want to view the dashboard from another device on the same network (e.g. a phone or tablet), replace "localhost" with the IP address of the PC running the server. For example: http://192.168.1.45:3000

7.2  Dashboard Pages
7.2.1  Overview Page
This is the default page shown on load. It displays:
•	Three summary counters: Total Packages, Sorted, and Pending.
•	A live line chart showing packages processed per minute.
•	A recent activity log showing the last 10 scanned packages.
•	A scrollable list of the most recently scanned packages.

Clicking on any package entry in the list opens a popup showing full details including the UID, product name, shelf location, batch number, source, and timestamp.

7.2.2  Packages Page
Shows the complete list of all packages ever processed by the system, sorted newest first. A search bar at the top allows filtering by product name or UID. Clicking any row opens the detail popup.

7.2.3  Analytics Page
Shows five charts that update every 2 seconds:
•	Packages per Hour — a bar chart showing how many packages were processed each hour.
•	Status Breakdown — a doughnut chart showing the ratio of Sorted to Pending packages.
•	Packages per Minute (rolling window) — a live line chart.
•	Top Locations — a horizontal bar chart showing which shelves have received the most packages.
•	Sorted vs Pending Trend — a line chart comparing the two statuses over time.

7.3  Resetting the Database
⚠   WARNING
Resetting the database permanently deletes all package records. This cannot be undone.

Step 50.	In the dashboard sidebar, click the "⟳ Reset Database" button at the bottom.
Step 51.	A confirmation prompt will appear. Click OK to confirm.
Step 52.	All records will be deleted and all counters will reset to zero.

7.4  Switching Between Dark and Light Mode
Click the "☀ Toggle Mode" button in the sidebar to switch between dark mode (default) and light mode.

8.1  Pre-Start Checklist
•	All wiring connections are secure and match their labels.
•	The conveyor belt is clear — no packages sitting on it.
•	The shelves are accessible — nothing blocking the arm's path to any shelf.
•	The PC is connected to the same Wi-Fi network as the ESP32.
•	MongoDB is running on the PC (it starts automatically if configured as a service).
•	The backend server is running (node server.js executed in Command Prompt).
•	The web dashboard is open in a browser (http://localhost:3000).
