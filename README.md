# DIYPCR
DIY Thermal Cycler for PCR
PCR code for denaturing, annealing and extending DNA cycles.
iDP (duilio.peroni@gmail.com) may 2018
Derived from: Stacey Kuznetsov (stace@cmu.edu) and Matt Mancuso (mcmancuso@gmail.com) July 2012
V 1.0 01/06/2018
Arduino based harware
- K-type thermocouple sensor with MAX6675 A/D converter
- 2x 50W resistor heater
- 12V fan cooler
- Start/stop button (internal pullup -> active-low signal)
- RGB LED & I2C LCD local diagn
- CSV remote log

PCR Cycle
           |              N x Cycles          |
     |  H  |   D   | C |    A    |H|   eX  |H |C|
Td _ _ _ _ _________ _ _ _ _ _ _ _ _ _ _ _ _ _ _|_
     |    /|       |\  |         | |       | /| |
Te _ _ _ /_ _ _ _ _|_\_|_ _ _ _ _  _________/_|_|_
     |  /  |       |  \|         |/|       |\ | | 
Ta _ _ /_ _|_ _ _ _|_ _\_________/_|_ _ _ _|_\|_|_
     |/    |       |   |         | |       |  \ |
Tr __/ _ _ |_ _ _ _| _ | _ _ _ _ |_|_ _ _ _|_ |\__
     |     |       |   |         | |       |  | |
     
CSV log format:
<total_time>;<cycle_number>;<phase>;<phase_time>;<temperature>;<status><cr><lf>
<total_time>   :: HH:MM:SS
<cycle_number> :: 1..NUM_CYCLES
<phase>        :: I=Idle, H=heating, C=cooling, D=denaturation, A=annealing, E=extension, F= fault 
<phase_time>   :: HH:MM:SS
<temperature>  :: tt.tt (°C)
<status>       :: 0=ok, 1=operator stop, 2=sensor not working, 3=temp already over setpoint, 4=temp not stable

RGB diagn colors:
Idle=green, Heating=red, Cooling=blue, Denaturation=fuchsia, Annealing=cyan, eXtension=yellow, fault=white.

