from pathlib import Path
import numpy as np,json
from PIL import Image
root=Path(__file__).resolve().parents[4]
evidence=Path(__file__).resolve().parent
names={'title':'标题页','inventory':'背包','storage':'箱子','pause':'暂停界面','dialogue':'对话','map':'地图','skills':'技能树','journal':'任务栏'}
def mean11(x):
    s=np.pad(x,((1,0),(1,0)),mode='constant').cumsum(0).cumsum(1)
    return (s[11:,11:]-s[:-11,11:]-s[11:,:-11]+s[:-11,:-11])/121
out={}
for name,ref in names.items():
    a=np.asarray(Image.open(root/'ui pic'/f'{ref}.png').convert('RGB'),dtype=np.float64)/255
    b=np.asarray(Image.open(evidence/f'{name}.png').convert('RGB'),dtype=np.float64)/255
    mae=float(np.abs(a-b).mean())
    x=a.mean(2);y=b.mean(2);mx=mean11(x);my=mean11(y)
    vx=mean11(x*x)-mx*mx;vy=mean11(y*y)-my*my;c=mean11(x*y)-mx*my
    ssim=((2*mx*my+.0001)*(2*c+.0009)/((mx*mx+my*my+.0001)*(vx+vy+.0009))).mean()
    out[name]={'rgb_mean_absolute_error':round(mae,4),'local_grayscale_ssim':round(float(ssim),4)}
result={'accepted_95_percent':False,'method':'RGB MAE normalized to 0..1; local 11x11 box-window SSIM of RGB-mean grayscale, not a fidelity percentage. Dynamic data differs. HUD excluded from metric.','pages':out}
(evidence/'visual-comparison.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(out,indent=2))
