# TRISTAN
This code will control the linear actuators on board the first iteration of the TRISTAN payload.
 

Changelog:

2025/04/20 11:00 PM - Changed code to default to extending actuators (release ball lock pins) at power-on, changed BLE device name to "STAR Tristan"

2025/04/12 01:00 AM - Implemented abilty to trigger actuators based on external wireless input (based on Conor's bluetooth switch code)

2025/03/28 10:00 PM - Implemented ability to trigger the actuators based on external wired input using AA batteries (with noise resistance). Still need to test with the fluctus.

2025/03/27 11:00 PM - First commit, containing code that was able to extend/retract the actuators based at a fixed interval, and could measure but not react to external inputs.