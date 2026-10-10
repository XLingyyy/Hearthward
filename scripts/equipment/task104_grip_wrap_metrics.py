"""Offline fingertip radial/angle metrics; run source-rig validation first."""
import json
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[2]
out=ROOT/'.agent-local/qa/TASK-104/grip'
result=[]
for role in ('Hero','Brother'):
 d=json.loads((out/(role+'-full-fit.json')).read_text());rig=json.loads((out/(role+'-rig.json')).read_text());skin=np.array(d['skin']);a=np.array(d['axis_hand']);c=np.array(d['palm_hand_local'])*d['scale'];b=np.cross(a,[0,0,1]);b/=np.linalg.norm(b);B=np.array([b,np.cross(a,b)]).T
 radial=np.linalg.norm((skin-c)@B,axis=1);digit=[]
 for name in ('index','middle','ring','pinky','thumb'):
  ix=[i for i,w in enumerate(rig['meshes'][0]['weights']) if w and max(w,key=lambda x:x[1])[0]==name+'_03_r' and all(n not in ('thigh_r','thigh_twist_01_r') for n,v in w)]
  xy=(skin[ix].mean(0)-c)@B;angle=float(np.degrees(np.arctan2(xy[1],xy[0])))
  digit.append({'finger':name,'tip_surface_vertices':len(ix),'closest_tip_surface_to_axis_cm':float(radial[ix].min()),'median_tip_surface_to_axis_cm':float(np.median(radial[ix])),'tip_centre_angle_degrees':angle})
 angles=sorted((x['tip_centre_angle_degrees']+360)%360 for x in digit);gaps=np.diff(angles+[angles[0]+360]);span=360-max(gaps)
 result.append({'role':role,'digits':digit,'tip_centres_angular_span_degrees':float(span),'interpretation':'Radius is axis distance, not signed exact mesh contact; exterior fingertip surfaces remain around the handle rather than all opened to one side.'})
print(json.dumps(result,indent=2));(ROOT/'docs/qa/TASK-104/grip/finger-wrap-metrics.json').write_text(json.dumps(result,indent=2)+'\n')
