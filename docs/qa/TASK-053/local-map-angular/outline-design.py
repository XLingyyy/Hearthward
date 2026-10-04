"""Design angular cuts within the existing observed map, without editing art."""
import json
from pathlib import Path
import sys
import numpy as np

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]
sys.path.insert(0,str(GAME / 'scripts/ui'))
from audit_local_map import boundary_fraction

original = json.loads((QA / 'before/Resources__UI__interface.json.snapshot').read_text(encoding='utf-8-sig'))['localMap']
old = np.array(original['outlineFractions'])
outline = boundary_fraction(np.arange(48)*2*np.pi/48,old)
changes = {0:.94,4:.93,22:.915,24:.96,26:.91,33:.87,39:.945,42:.945,45:.925}
for index,value in changes.items():
    outline[index] = value
angular = [47,0,3,4,21,22,25,26,32,33,38,39,43,44,45]
feather = np.ones(48)
for index in angular:
    feather[index] = feather[(index+1)%48] = .4
for index in [0,4,22,26,33,39,45]:
    feather[index] = .28

angles = np.arange(32768)*2*np.pi/32768
sample = angles*48/(2*np.pi)
index = np.floor(sample).astype(int)
step = 2*np.pi/48
ax,ay = outline[index]*np.cos(index*step),outline[index]*np.sin(index*step)
bx,by = outline[(index+1)%48]*np.cos((index+1)*step),outline[(index+1)%48]*np.sin((index+1)*step)
straight = (ax*(by-ay)-ay*(bx-ax))/(np.cos(angles)*(by-ay)-np.sin(angles)*(bx-ax))
radii = np.where(np.isin(index,angular),straight,boundary_fraction(angles,outline))
ratio = float(np.mean(radii**2))
result = dict(outlineFractions=outline.tolist(),outlineAngularSegments=angular,outlineFeatherFractions=feather.tolist(),
    area_ratio=ratio,area_m2=float(np.pi*250**2*ratio),radius_range_m=[float(radii.min()*250),float(radii.max()*250)],
    angular_percent=100*len(angular)/48,scope='Straight Cartesian edge sections joined to smooth sections; local feather narrowed around corners, original 250 m envelope preserved.')
(QA/'outline-plan.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({key:result[key] for key in ['area_ratio','area_m2','radius_range_m','angular_percent']},indent=2))
assert ratio>=.90 and radii.min()>.8 and radii.max()<=1
