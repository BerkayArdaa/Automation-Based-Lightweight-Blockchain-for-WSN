This is WSN Project to Define Security and Self Harvesting
Project Proposal: Automation-Based Lightweight Blockchain Framework for Data Integrity in Wireless
Sensor Networks
Problem to be addressed:
Wireless Sensor Networks (WSNs) are commonly used for environmental monitoring, agriculture and smart city
systems. However, these nodes use wireless communication and they have limited processing power. For this
reason, attackers can sniff, modify or replay transmitted data. Existing encryption and authentication methods
protect individual links but cannot guarantee end-to-end data integrity or prevent tampering after data leaves the
source node. Moreover, sending every measurement to a blockchain causes high energy consumption and
latency in resource-limited WSNs.
Proposed approach:
This project suggests a lightweight blockchain system with an automation layer to keep data safe and save
energy in WSNs. Cluster-head (CH) nodes will collect data from sensors, create Merkle roots, and store them in
a secure permissioned blockchain. Instead of adding blocks at fixed times, the automation layer will decide
when to add new blocks by checking real-time conditions such as node energy, data traffic, and possible attacks.
This smart process helps reduce extra blockchain work while keeping the data trustworthy and protected from
changes.
Citations
[1] S. Ismail, D. W. Dawoud, and H. Reza, “Securing Wireless Sensor Networks Using Machine Learning and
Blockchain: A Review,” Future Internet, vol. 15, no. 6, p. 200, 2023, doi: 10.3390/fi15060200
https://www.mdpi.com/1999-5903/15/6/200
[2] I. A. A. E.-M. And and S. M. Darwish, "Towards Designing a Trusted Routing Scheme in Wireless Sensor
Networks: A New Deep Blockchain Approach," IEEE Access, vol. 9, pp. 103822–103834, 2021, doi:
10.1109/ACCESS.2021.3098933.
https://ieeexplore.ieee.org/document/9492073
[3] L. García, C. Cancimance, R. Asorey-Cacheda, C.-L. Zúñiga-Cañón, A.-J. Garcia-Sanchez, and J.
Garcia-Haro, "Lightweight Blockchain for Data Integrity and Traceability in IoT Networks," IEEE Access, vol.
13, pp. 81105–81117, 2025, doi: 10.1109/ACCESS.2025.356777.
https://ieeexplore.ieee.org/document/10990139
Why our proposed solution is different or better:
Previous studies used blockchain in sensor networks but in a static or limited way. For example, Ismail et al. [1]
only discussed blockchain and machine learning uses conceptually without any adaptive control. El-Moghith
and Darwish [2] focused on routing trust instead of data integrity or energy efficiency. García et al. [3] built a
lightweight blockchain for IoT, but blocks were created at fixed time intervals.
Our project adds an automation layer that decides when to record new blocks based on real-time energy, traffic,
and anomaly data. This makes the system more adaptive and efficient—saving energy, reducing delay, and
keeping data secure and traceable beyond what these static methods achieved.
