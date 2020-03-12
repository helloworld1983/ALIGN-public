************************************************************************
* auCdl Netlist:
* 
* Library Name:  OTA_class
* Top Cell Name: single_ended_low_voltage_cascode_current_mirror
* View Name:     schematic
* Netlisted on:  Sep 11 21:08:07 2019
************************************************************************

*.BIPOLAR
*.RESI = 2000 
*.RESVAL
*.CAPVAL
*.DIOPERI
*.DIOAREA
*.EQUATION
*.SCALE METER
*.MEGA
.PARAM

*.GLOBAL vdd!
+        gnd!

*.PIN vdd!
*+    gnd!

************************************************************************
* Library Name: OTA_class
* Cell Name:    single_ended_low_voltage_cascode_current_mirror
* View Name:    schematic
************************************************************************

.SUBCKT single_ended_low_voltage_cascode_current_mirror Vbiasn Vinn Vinp Voutp
*.PININFO Vbiasn:I Vinn:I Vinp:I Voutp:O
MM3 Voutp Vinp net13 gnd! nmos w=WA l=LA nfin=nA
MM0 net11 Vinn net13 gnd! nmos w=WA l=LA nfin=nA
MM4 net13 Vbiasn gnd! gnd! nmos w=WA l=LA nfin=nA
MM6 net11 net18 net19 vdd! pmos w=WA l=LA nfin=nA
MM5 Voutp net18 net20 vdd! pmos w=WA l=LA nfin=nA
MM1 net20 net11 vdd! vdd! pmos w=WA l=LA nfin=nA
MM2 net19 net11 vdd! vdd! pmos w=WA l=LA nfin=nA
.ENDS


.SUBCKT LG_nmos Biasp Vbiasn
*.PININFO Biasp:I Vbiasn:O
MM8 Vbiasn Vbiasn gnd! gnd! nmos w=WA l=LA nfin=nA
MM10 Vbiasn Biasp vdd! vdd! pmos w=WA l=LA nfin=nA
.ENDS

.SUBCKT CR15_1 Vbiasp
*.PININFO Vbiasp:O
RR1 Vbiasp gnd! res=rK
MM2 Vbiasp Vbiasp vdd! vdd! pmos w=WA l=LA nfin=nA
.ENDS


xiota LG_Vbiasn Vinn Vinp Voutp single_ended_low_voltage_cascode_current_mirror
xiLG_nmos Biasp LG_Vbiasn LG_nmos
xibCR15_1 Biasp CR15_1
.END