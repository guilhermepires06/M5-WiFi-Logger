📡 M5Stack Cardputer Wi-Fi Hotspot

Transform your M5Stack into a simulated public Wi-Fi hotspot with custom authentication and data logging!
This project demonstrates how to create a public hotspot using the M5Stack Cardputer. It simulates a Wi-Fi network and serves a custom web page hosted on an SD card. User-submitted data is stored locally on the SD card, offering complete control and privacy for testing and educational purposes.
Ideal for educational applications and IoT projects, the system combines efficient hardware with a simple and functional interface.
__________________________________________________________________________________________________________________________________________________________________________
🚀 Key Features

•	Simulated Wi-Fi Hotspot: Create a public network simulating Facebook authentication.
•	Custom Web Page: The page displayed to users is loaded directly from the SD card.
•	Credential Logging: All user-submitted data is saved in the senhas.txt file.
•	Local Control: Use the mapped M5Stack keyboard to navigate the menu and manage connections.
•	Real-Time Monitoring: View network status, including the number of connected devices and logged credentials.
__________________________________________________________________________________________________________________________________________________________________________
🎛️ Requirements

•	Microcontroller: M5Stack Cardputer or equivalent with SD card support.
•	SD Card: Required to store the web page, configuration files, and logs.
•	Arduino IDE: Development tool to upload the code to the M5Stack.
•	Mapped Keyboard: For navigating the menu and managing the interface.
__________________________________________________________________________________________________________________________________________________________________________
🛠️ How to Use

1.	Download this repository.
2.	Upload the code in the src folder to the M5Stack using the Arduino IDE.
3.	Copy the files from the sd_content folder to the SD card.
4.	Insert the SD card into the M5Stack and power it on.
5.	Connect to the Wi-Fi network created by the device.
6.	When accessing the simulated web page, any data submitted will be saved locally on the SD card.
__________________________________________________________________________________________________________________________________________________________________________
MIT License

Copyright (c) 2024 Guilherme


Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
__________________________________________________________________________________________________________________________________________________________________________
Ethical Note
This project was developed for educational and demonstration purposes only.
It must not be used to collect user data without explicit consent or to violate the privacy of others.
The author is not responsible for any misuse of this software.
