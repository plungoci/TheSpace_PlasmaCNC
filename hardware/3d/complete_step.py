from OCP.STEPCAFControl import STEPCAFControl_Reader, STEPCAFControl_Writer
from OCP.TDocStd import TDocStd_Document
from OCP.TCollection import TCollection_ExtendedString
from OCP.XCAFDoc import XCAFDoc_DocumentTool, XCAFDoc_ColorType
from OCP.TDF import TDF_LabelSequence, TDF_Label
from OCP.TDataStd import TDataStd_Name
from OCP.Quantity import Quantity_Color, Quantity_TOC_RGB
from OCP.Bnd import Bnd_Box
from OCP.BRepBndLib import BRepBndLib
from OCP.STEPControl import STEPControl_AsIs
from OCP.IFSelect import IFSelect_RetDone
import cadquery as cq

SRC='/tmp/step/LS ESP32 PRO V2.1_002.step'
doc=TDocStd_Document(TCollection_ExtendedString('XmlOcaf'))
r=STEPCAFControl_Reader(); r.SetColorMode(True); r.SetNameMode(True)
assert r.ReadFile(SRC)==IFSelect_RetDone; r.Transfer(doc)
st=XCAFDoc_DocumentTool.ShapeTool_s(doc.Main()); ct=XCAFDoc_DocumentTool.ColorTool_s(doc.Main())
free=TDF_LabelSequence(); st.GetFreeShapes(free); root=free.Value(1)
print('free shapes',free.Length(),'assembly',st.IsAssembly_s(root))

def bbox(shape):
    b=Bnd_Box(); BRepBndLib.Add_s(shape,b); return b.Get()

# remove the JST-XH 4 pin drawn at J12 (the board has the RJ jack J14 there)
comps=TDF_LabelSequence(); st.GetComponents_s(root,comps,True)
removed=0
for i in range(1,comps.Length()+1):
    l=comps.Value(i); s=st.GetShape_s(l)
    x0,y0,z0,x1,y1,z1=bbox(s)
    if abs(x0+80.86)<0.3 and abs(y0-8.1)<0.3 and abs(x1+68.46)<0.3:
        st.RemoveComponent(l); removed+=1
print('removed J12 JST:',removed)

def add(name, wp, rgb):
    from OCP.BRepBuilderAPI import BRepBuilderAPI_Copy
    shape=wp.val().wrapped if hasattr(wp,'val') else wp.wrapped
    from OCP.BRepBuilderAPI import BRepBuilderAPI_Transform
    from OCP.gp import gp_Trsf
    shape=BRepBuilderAPI_Transform(shape,gp_Trsf(),True).Shape()  # bake location into geometry
    lab=st.AddShape(shape,False)
    TDataStd_Name.Set_s(lab,TCollection_ExtendedString(name))
    c=st.AddComponent(root,lab,shape.Location()); assert not c.IsNull(); TDataStd_Name.Set_s(c,TCollection_ExtendedString(name))
    ct.SetColor(lab,Quantity_Color(*rgb,Quantity_TOC_RGB),XCAFDoc_ColorType.XCAFDoc_ColorSurf)

def box(x0,x1,y0,y1,z0,z1):
    return cq.Workplane().box(x1-x0,y1-y0,z1-z0,centered=False).translate((x0,y0,z0))

BT=1.65   # board top
# 1) J25 lower row: 1x08 female header, 12.7 mm below the existing row
X0,X1=-48.81,-28.49
sock=box(X0,X1,22.72,25.26,BT,8.65)
for k in range(8):
    cx=X0+1.27+k*2.54
    sock=sock.cut(box(cx-0.5,cx+0.5,23.49,24.49,6.0,8.66))
add('J25_PinSocket_1x08_lower_row',sock,(0.1,0.1,0.1))
# 2) TMC2209 StepStick module on J25 (15.24 x 20.32, PCB on top of the sockets) + heatsink
yc=(24.0+36.69)/2
pcb=box(X0,X1,yc-7.62,yc+7.62,8.65+2.5,8.65+2.5+1.6)      # ~2.5 mm male pin plastic + PCB
pcb=pcb.cut(box(X0+0.6,X0+1.6,yc-7.0,yc-6.0,11.0,13.0))  # pin-1 mark
add('TMC2209_module_PCB_on_J25',pcb,(0.05,0.35,0.15))
hz=8.65+2.5+1.6
hs=box(-38.65-4.5,-38.65+4.5,yc-4.5,yc+4.5,hz+1,hz+1+7.0)
for k in range(4):
    gx=-38.65-4.5+1.2+k*2.2
    hs=hs.cut(box(gx,gx+1.0,yc-4.6,yc+4.6,hz+3,hz+8.1))
add('TMC2209_module_heatsink_9x9x7',hs,(0.75,0.75,0.78))
# 3) heatsinks on the on-board X/Y TMC2209 (QFN), ~9 x 11 x 6 mm (assumed)
for nm,(qx0,qx1,qy0,qy1) in {'X':(-80.8,-75.8,27.94,32.94),'Y':(-60.96,-55.96,27.89,32.89)}.items():
    cx,cy=(qx0+qx1)/2,(qy0+qy1)/2
    h=box(cx-4.5,cx+4.5,cy-5.5,cy+5.5,2.6,8.6)
    for k in range(4):
        gx=cx-4.5+1.2+k*2.2
        h=h.cut(box(gx,gx+1.0,cy-5.6,cy+5.6,4.0,8.7))
    add(f'{nm}_driver_heatsink_9x11x6_assumed',h,(0.08,0.08,0.08))
# 4) J14 pendant jack, 4P4C (RJ9/RJ10), front at the board edge
rj=box(-79.85,-69.45,-2.0,11.0,BT,BT+10.0).cut(box(-77.6,-71.7,-2.1,6.0,BT+2.0,BT+8.0))
add('J14_RJ_4P4C_pendant',rj,(0.1,0.1,0.1))
# 5) laser output "V-TTL-s" 3 pin MR30 type (yellow), vertical
mr=box(-96.6,-84.4,44.4,49.7,BT,BT+9.0)
for px in (-94.2,-90.5,-86.8):
    mr=mr.cut(box(px-1.2,px+1.2,45.9,48.3,BT+5.0,BT+9.1))
add('J_V-TTL-s_MR30_3pin',mr,(0.95,0.75,0.1))

st.UpdateAssemblies()
w=STEPCAFControl_Writer(); w.SetColorMode(True); w.SetNameMode(True)
w.Transfer(doc,STEPControl_AsIs)
w.Write('/tmp/stepwork/LS_ESP32_PRO_V2.1_002_completed.step')
print('written')
