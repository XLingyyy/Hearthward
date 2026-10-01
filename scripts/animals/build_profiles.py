"""Bind per-species behavior plans to the actual imported paired assets."""
import json
from pathlib import Path
GAME=Path(__file__).resolve().parents[2]
OUT=GAME/'docs/qa/TASK-051'
def step(clip,minimum=0,maximum=0):return {'clip':clip,'min_seconds':minimum,'max_seconds':maximum}
def cycle(*clips):return [step(c) if isinstance(c,str) else c for c in clips]
def forage(prefix):return cycle(prefix+'_In',step(prefix+'_Loop',6,10),prefix+'_Out')
def rest(down='LieDown',idle='Rest'):return cycle(down,step(idle,10,18),'GetUp')
DEFS={
 'stag_a':('雄鹿A','Idle','Walk','Gallop','StartRun','Stop','Alert',1000,[forage('Graze'),cycle('LookAround'),rest()]),
 'hare':('野兔','IdleCrouch','HopSlow','BoundFast','StartBound','StopCrouch','Alert',700,[forage('Nibble'),cycle('Groom'),cycle('Scan'),cycle(step('Rest',8,14))]),
 'goat':('山羊','Idle','Walk','RunFlee','Start','Stop','Alert',800,[forage('Graze'),cycle('Chew'),cycle('BrowseLook'),rest()]),
 'pheasant':('雉鸡','Idle','Walk','FlutterRun','StartRun','Stop','Alert',600,[cycle(step('Peck',4,7),'Scratch'),cycle('Preen','Shake'),cycle('DustBath')]),
 'pig':('猪','IdleDomestic','Walk','RunDomestic','Start','Stop','AlertWild',700,[cycle('RootSniff',*forage('Eat')),rest()]),
 'wolf':('狼','Idle','Walk','Gallop','StartRun','Stop','AlertThreat',1000,[cycle('SniffGround'),rest(),cycle('Howl')]),
 'black_bear':('黑熊','Idle','WalkHeavy','Lope','StartRun','StopBrace','AlertHuff',1100,[cycle('SniffForage','DigVisual'),cycle('RearSniff_In',step('RearSniff_Hold',3,5),'RearSniff_Out'),rest()]),
 'ram':('公羊','Idle','Walk','RunClose','Start','Stop','AlertHeadLow',800,[forage('Graze'),cycle('Chew'),rest()]),
 'hen':('母鸡','Idle','Walk','EscapeFlutter','Start','Stop','Alert',400,[cycle(step('Peck',4,7),'ScratchPeck'),cycle('Preen','Shake'),cycle('DustBath_In',step('DustBath_Loop',4,7),'DustBath_Out'),cycle('NestSettle',step('NestRest',8,12),'NestExit')]),
 'red_fox':('赤狐','Idle','Walk','RunFlee','StartRun','Stop','AlertListen',900,[cycle('Sniff','PounceVisual'),cycle('Groom'),rest('CurlDown','CurlRest')]),
 'carp':('鲤鱼','Hover','SwimSlow','SwimCruise','Burst','Brake','Hover',400,[cycle('Bite_In'),cycle(step('Coast',3,5))]),
 'crucian_carp':('鲫鱼','Hover','SwimSlow','SwimCruise','Burst','Brake','Hover',350,[cycle('Bite_In'),cycle(step('PauseGlide',3,5))]),
 'catfish':('鲶鱼','HoverLow','SwimSlow','SwimCruise','Burst','Brake','HoverLow',400,[cycle('WhiskerSearch','Bite_In'),cycle(step('HoverLow',4,7))]),
 'eel':('鳗鱼','HoverWave','UndulateSlow','UndulateFast','StartWave','StopWave','HoverWave',400,[cycle('EscapeHide'),cycle('Bite_In'),cycle(step('HoverWave',4,7))])
}
def main():
    report=json.loads((OUT/'import_report.json').read_text('utf-8'));assert report['ok']
    gameplay=json.loads((GAME/'Resources/Data/gameplay.json').read_text('utf-8'))
    player_sprint=gameplay['tuning']['sprintSpeed']
    escape_multiplier=1.05
    escape_speed=player_sprint*escape_multiplier
    species=[]
    for a in report['animals']:
        slug=a['slug'];name,idle,walk,run,start,stop,alert,detect,cycles=DEFS[slug]
        aquatic=slug in ('carp','crucian_carp','catfish','eel');clips={c['suffix']:c for c in a['clips']}
        walk_reference=clips[walk]['reference_speed_cm_s'] or (25 if aquatic else 90)
        run_reference=clips[run]['reference_speed_cm_s'] or (180 if aquatic else 450)
        for plan in cycles:
            for s in plan:
                # Accept nested steps when composing a three-part eating cycle.
                if isinstance(s['clip'],dict):s.update(s['clip'])
                assert s['clip'] in clips,(slug,s)
        row={'slug':slug,'name':name,'aquatic':aquatic,'mesh':a['mesh']['primary_asset']['path'],'skeleton':a['skeleton'],'clips':clips,'idle':idle,'walk':walk,'run':run,'start':start,'stop':stop,'alert':alert,'hit':'StruggleS' if slug=='eel' else 'Struggle' if aquatic else 'Hit_L','collapse':'SettleArc' if slug=='eel' else 'Settle' if aquatic else 'Collapse_L','corpse':'DisplayStill' if aquatic else 'CorpseHold_L','detect_radius_cm':detect,'natural_cycles':cycles,'walk_speed_cm_s':clips[walk]['reference_speed_cm_s'] or (25 if aquatic else 90),'run_speed_cm_s':escape_speed,'source_blend_sha256':a['source_blend_sha256'],'skeletal_sha256':a['skeletal_sha256']}
        row['walk_reference_speed_cm_s']=walk_reference
        row['run_reference_speed_cm_s']=run_reference
        for role in ('idle','walk','run','alert','hit','collapse','corpse'):
            assert row[role] in clips,(slug,role)
        assert clips[run]['loop'],(slug,'sustained escape must keep animating')
        species.append(row)
    result={'schema_version':1,'revision':'2026-10-01 R3 sprint balance','flee_balance':{'player_base_sprint_cm_s':player_sprint,'sprint_multiplier':escape_multiplier,'player_sprint_skills_apply':True},'species':species,'timing_note':'Natural durations, detection distances and movement speeds are configurable gameplay presentation values, not measured zoological schedules.'}
    (GAME/'Resources/Data/animal_motion.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print('PROFILES_BOUND',len(species),sum(len(s['clips']) for s in species))
if __name__=='__main__':main()
