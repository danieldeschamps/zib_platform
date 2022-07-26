# ZiB Platform
A complete Zigbee Platform Implementation.

Low power and battery operated, IoT Zigbee nodes communicate with real world sensors and actuators. 
Through a wireless mesh network, information is routed to the gateway device.
The gateway is responsible to store the data and provide a web service that hosts the platform web interface.

![alt text](https://github.com/danieldeschamps/zib_platform/blob/main/ZiB%20Platform.jpg)

![alt text](https://github.com/danieldeschamps/zib_platform/blob/main/ZiB%20DTP.jpg)

# The Project
It was developed as an initiative to seek for venture capital in order to start a tech company using IoT concept en early stages of the industry

# The code
- zib_manager is gateway C++ code that compiles with MSVC and run on windows as a system service
- zib_network is the nodes C firmware that run on the nodes harware. 
- zib_network\ZiBDTP is the code for endpoint nodes
- zib_network\ZiBGateway is the code for the gateway USB board that enables the windows server to communicate with the ZigBee network.
- sql_firebird is the basic database SQL code

# The web interface
The ASP.net web interface was excluded from this repository as it was considered deprecated.
The SQL database is the representation of what the web interface reads and writes through Microsoft IIS.

# The hardware
The hardware schematics and gerber files are not part of this repository
