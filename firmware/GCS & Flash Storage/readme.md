##  GROUND CONTROL STATION  & FLASH STORAGE MODULE

1. SYSTEM OVERVIEW
------------------
The GCS subsystem runs an asynchronous web server hosting a tuning interface 
on the ESP32 soft Access Point[cite: 18]. Control loop gains modified via the interface 
are injected into dynamic RAM and saved to non-volatile SPIFFS flash storage[cite: 18].

2. PARAMETER PERSISTENCE ARCHITECTURE
-------------------------------------
Gain parameter updates are submitted via HTTP GET requests and routed through 
SPIFFS persistent storage[cite: 18]:

$$\text{Web Client Input} \xrightarrow{\text{HTTP GET}} \text{Async Handler} \xrightarrow{\text{RAM Update}} \text{SPIFFS Writes (/pRP.txt, etc.)}$$

Upon system initialization, flash files are parsed to populate control registers:

$$K_{\text{param}} = \begin{cases} 
\text{StorageVal} & \text{if FileExists(SPIFFS)} \\ 
\text{DefaultVal} & \text{otherwise} 
\end{cases}$$

3. NETWORK HARDWARE CONFIGURATION
---------------------------------
  * Access Point SSID : ALPHA-S_GCS[cite: 18]
  * Access Point Pass : 909090[cite: 18]
  * Server IP Address : 192.168.4.1[cite: 18]
  * Listening Port    : 80 (HTTP)[cite: 18]
