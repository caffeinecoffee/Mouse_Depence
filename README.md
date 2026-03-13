Defender Program (Mouse Behavior Defense System)
- Overview
This project implements a mouse behavior–based defense system designed to protect against automated attacks targeting mouse authentication systems.
The defender module generates GAN-like synthetic mouse coordinates and introduces dynamic interaction noise to prevent attackers from accurately reproducing legitimate mouse behavior patterns.
The system records real mouse movements and clicks while simultaneously injecting synthetic behavioral data to confuse automated attack programs attempting to mimic user behavior.
This research aims to enhance the robustness of behavioral biometric authentication systems against automated replay or imitation attacks.

⚙ Key Features
- Mouse Event Monitoring
Captures real-time mouse movements and click events using Windows hook APIs.
- Synthetic Mouse Coordinate Generation
Generates artificial mouse coordinates similar to GAN-generated data.
- Behavior Visualization
Displays generated coordinates and user mouse activity inside a Picture Control UI.
- IPC Communication
Sends generated coordinates to the attacker simulation through Named Pipe communication.
- Mouse Activity Logging
Records mouse coordinates, timestamps, and click intervals in CSV format for behavioral analysis.


Research Purpose
This project is designed for security research and experimentation on mouse dynamics authentication systems.
Main research topics include:
  - Behavioral biometric security
  - Mouse dynamics authentication
  - Automated attack simulation
  - Defense against behavior replay attacks
  - Human vs synthetic input differentiation


🖥 Development Environment
- Language: C++
- Framework: MFC (Microsoft Foundation Classes)
- OS: Windows
- IDE: Visual Studio 2022
- Communication: Named Pipe IPC
