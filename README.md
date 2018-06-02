# DIYPCR
DIY Thermal Cycler for PCR<br>
PCR code for denaturing, annealing and extending DNA cycles.<br>
iDP (duilio.peroni@gmail.com) may 2018<br>
Derived from: Stacey Kuznetsov (stace@cmu.edu) and Matt Mancuso (mcmancuso@gmail.com) July 2012<br>
V 1.0 01/06/2018<br>
Arduino based harware
- K-type thermocouple sensor with MAX6675 A/D converter
- 2x 50W resistor heater
- 12V fan cooler
- Start/stop button (internal pullup -> active-low signal)
- RGB LED & I2C LCD local diagn
- CSV remote log

PCR Cycle
<pre>
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
</pre>     
CSV log format:
<total_time>;<cycle_number>;<phase>;<phase_time>;<temperature>;<status><cr><lf><br>
<total_time>   :: HH:MM:SS<br>
<cycle_number> :: 1..NUM_CYCLES<br>
<phase>        :: I=Idle, H=heating, C=cooling, D=denaturation, A=annealing, E=extension, F= fault<br> 
<phase_time>   :: HH:MM:SS<br>
<temperature>  :: tt.tt (°C)<br>
<status>       :: 0=ok, 1=operator stop, 2=sensor not working, 3=temp already over setpoint, 4=temp not stable<br>
RGB diagn colors:<br>
Idle=green, Heating=red, Cooling=blue, Denaturation=fuchsia, Annealing=cyan, eXtension=yellow, fault=white.<br>

