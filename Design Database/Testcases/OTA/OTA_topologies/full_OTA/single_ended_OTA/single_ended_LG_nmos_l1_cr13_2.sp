************************************************************************
* auCdl Netlist:
* 
* Library Name:  OTA_class
* Top Cell Name: single_ended
* View Name:     schematic
* Netlisted on:  Sep 11 21:06:14 2019
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
* Cell Name:    single_ended
* View Name:    schematic
************************************************************************

.SUBCKT single_ended Vbiasn Vinn Vinp Voutp
*.PININFO Vbiasn:I Vinn:I Vinp:I Voutp:O
MM3 Voutp Vinp net16 gnd! nmos_rvt w=WA l=LA nfin=nA
MM0 Vbiasp Vinn net16 gnd! nmos_rvt w=WA l=LA nfin=nA
MM4 net16 Vbiasn gnd! gnd! nmos_rvt w=WA l=LA nfin=nA
MM1 Voutp Vbiasp vdd! vdd! pmos_rvt w=WA l=LA nfin=nA
MM2 Vbiasp Vbiasp vdd! vdd! pmos_rvt w=WA l=LA nfin=nA
.ENDS


.SUBCKT LG_nmos_l1 Biasn Vbiasn Vbiasp
*.PININFO Biasn:I Vbiasn:O Vbiasp:O
MM2 Vbiasn Vbiasn gnd! gnd! nmos_rvt w=WA l=LA nfin=nA
MM10 Vbiasp Biasn gnd! gnd! nmos_rvt w=WA l=LA nfin=nA
MM1 Vbiasn Vbiasp vdd! vdd! pmos_rvt w=WA l=LA nfin=nA
MM0 Vbiasp Vbiasp vdd! vdd! pmos_rvt w=WA l=LA nfin=nA
.ENDS

.SUBCKT CR13_2 Vbiasn Vbiasp
*.PININFO Vbiasn:O Vbiasp:O
MM2 Vbiasp Vbiasn gnd! gnd! nmos_rvt w=WA l=LA nfin=nA
MM0 Vbiasn Vbiasn gnd! gnd! nmos_rvt w=WA l=LA nfin=nA
MM3 Vbiasp Vbiasp vdd! vdd! pmos_rvt w=WA l=LA nfin=nA
MM1 Vbiasn Vbiasp vdd! vdd! pmos_rvt w=WA l=LA nfin=nA
.ENDS


xiota LG_Vbiasn Vinn Vinp Voutp single_ended
xiLG_nmos_l1 Biasn LG_Vbiasn LG_Vbiasp LG_nmos_l1
xibCR13_2 Biasn Vbiasp CR13_2
.END