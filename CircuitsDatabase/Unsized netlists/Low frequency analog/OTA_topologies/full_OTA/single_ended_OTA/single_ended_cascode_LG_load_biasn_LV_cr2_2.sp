************************************************************************
* auCdl Netlist:
* 
* Library Name:  OTA_class
* Top Cell Name: single_ended_cascode
* View Name:     schematic
* Netlisted on:  Sep 11 21:06:28 2019
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
* Cell Name:    single_ended_cascode
* View Name:    schematic
************************************************************************

.SUBCKT single_ended_cascode Vbiasn1 Vbiasn2 Vbiasp1 Vbiasp2 Vinn Vinp Voutn
*.PININFO Vbiasn1:I Vbiasn2:I Vbiasp1:I Vbiasp2:I Vinn:I Vinp:I Voutn:O
MM10 net27 net12 gnd! gnd! nmos_rvt w=WA l=LA nfin=nA
MM9 net21 net12 gnd! gnd! nmos_rvt w=WA l=LA nfin=nA
MM8 net12 Vbiasn2 net27 gnd! nmos_rvt w=WA l=LA nfin=nA
MM7 Voutn Vbiasn2 net21 gnd! nmos_rvt w=WA l=LA nfin=nA
MM3 net32 Vinp net10 gnd! nmos_rvt w=WA l=LA nfin=nA
MM0 net26 Vinn net10 gnd! nmos_rvt w=WA l=LA nfin=nA
MM4 net10 Vbiasn1 gnd! gnd! nmos_rvt w=WA l=LA nfin=nA
MM6 net12 Vbiasp2 net26 vdd! pmos_rvt w=WA l=LA nfin=nA
MM5 Voutn Vbiasp2 net32 vdd! pmos_rvt w=WA l=LA nfin=nA
MM1 net32 Vbiasp1 vdd! vdd! pmos_rvt w=WA l=LA nfin=nA
MM2 net26 Vbiasp1 vdd! vdd! pmos_rvt w=WA l=LA nfin=nA
.ENDS


.SUBCKT LG_load_biasn_LV Vbiasn2 Biasp
*.PININFO Biasp:I Vbiasn2:O
MM13 net9 Vbiasn2 gnd! gnd! nmos_rvt w=WA l=LA nfin=nA
MM15 Vbiasn2 Vbiasn2 net9 gnd! nmos_rvt w=WA l=LA nfin=nA
MM14 Vbiasn2 Biasp vdd! vdd! pmos_rvt w=WA l=LA nfin=nA
.ENDS

.SUBCKT CR2_2_wilson Vbiasn Vbiasp
*.PININFO Vbiasn:O Vbiasp:O
RR0 Vbiasn gnd! res=rK
MM2 Vbiasp net12 Vbiasn gnd! nmos_rvt w=WA l=LA nfin=nA
MM0 net12 Vbiasn gnd! gnd! nmos_rvt w=WA l=LA nfin=nA
MM3 Vbiasp Vbiasp vdd! vdd! pmos_rvt w=WA l=LA nfin=nA
MM1 net12 Vbiasp vdd! vdd! pmos_rvt w=WA l=LA nfin=nA
.ENDS


xiota LG_Vbiasn1 LG_Vbiasn2 LG_Vbiasp1 LG_Vbiasp2 Vinn Vinp Voutn single_ended_cascode
xiLG_load_biasn_LV Biasp LG_Vbiasn2 LG_load_biasn_LV
xibCR2_2_wilson Biasn Biasp CR2_2_wilson
.END