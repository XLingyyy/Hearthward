from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
import json
r=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果\corrections_20261001_r2');font=ImageFont.truetype(r'C:\Windows\Fonts\msyh.ttc',23)
sheet=Image.new('RGB',(3*560,2*460),'#e8ecef');d=ImageDraw.Draw(sheet)
for i,(slug,name) in enumerate([('stag_a','雄鹿A'),('ram','公羊'),('red_fox','赤狐')]):
 for row,tag in [(0,'修正前'),(1,'修正后')]:
  p=r/'inspection/before'/slug/'head_front.png' if row==0 else r/'inspection'/f'{slug}_corrected_head_front.png'
  im=Image.open(p).convert('RGB');im.thumbnail((550,420));sheet.paste(im,(i*560+(560-im.width)//2,row*460+40));d.text((i*560+16,row*460+8),name+' · '+tag,font=font,fill='#152d3b')
sheet.save(r/'头部修正前后.png')
videos=json.loads((r/'video_encoding_qa.json').read_text('utf-8'))
for batch,animals in enumerate([['stag_a','hare','goat','pig'],['wolf','black_bear','ram','red_fox']]):
 sheet=Image.new('RGB',(8*240,4*210),'#e8ecef');d=ImageDraw.Draw(sheet)
 for row,slug in enumerate(animals):
  record=next(v for v in videos if v['slug']==slug and v['category']=='correction_preview' and v['name'].endswith('_side'))
  for col,item in enumerate(record['samples']):
   im=Image.open(item['path']).convert('RGB').resize((240,180));sheet.paste(im,(col*240,row*210+30))
   d.text((col*240+6,row*210+5),f'{slug} {item["time_s"]:.2f}s',font=ImageFont.truetype(r'C:\Windows\Fonts\arial.ttf',16),fill='#152d3b')
 sheet.save(r/f'步态连续检查_{batch+1}.png')
record=[v for v in videos if v['slug']=='goat' and v['category']=='correction_preview' and v['name'].startswith('rest_sequence')]
sheet=Image.new('RGB',(8*240,2*230),'#e8ecef');d=ImageDraw.Draw(sheet)
for row,v in enumerate(record):
 for col,item in enumerate(v['samples']):
  im=Image.open(item['path']).convert('RGB').resize((240,180));sheet.paste(im,(col*240,row*230+40));d.text((col*240+6,row*230+5),f'{v["name"]} {item["time_s"]:.2f}s',font=ImageFont.truetype(r'C:\Windows\Fonts\arial.ttf',14),fill='#152d3b')
sheet.save(r/'山羊卧姿连续检查.png')

