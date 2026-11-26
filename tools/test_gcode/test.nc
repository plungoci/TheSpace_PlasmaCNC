(TheSpace PlasmaCNC - pendant test job)
(Rectangle 50 x 30 mm + circle R10, about 20 s at F1000)
(NO M3 / NO spindle-torch command - safe without anything connected)
(No Z moves. Expected frame bounds: X10 Y10 - X60 Y40)
G21
G90
G0 X10 Y10
G1 X60 Y10 F1000
G1 X60 Y40
G1 X10 Y40
G1 X10 Y10
G0 X45 Y25
G2 X45 Y25 I-10 J0 F1000
G0 X0 Y0
M2
