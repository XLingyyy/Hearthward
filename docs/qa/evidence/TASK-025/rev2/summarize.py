"""Compute delivery metrics from retained trajectories; never promote a guardrail correction to model success."""
from pathlib import Path
import json,math,statistics,re
ev=Path(__file__).resolve().parent
actions={'collect','craft','craft_rope','repair'}
def read(name):
    p=ev/name
    return json.loads(p.read_text(encoding='utf-8-sig')) if p.exists() else None
def ratio(rows,key='pass'):
    return {'success':sum(bool(x[key]) for x in rows),'total':len(rows),'rate':sum(bool(x[key]) for x in rows)/len(rows) if rows else None}
def distribution(v):
    return {'n':len(v),'min':min(v),'median':statistics.median(v),'p95':sorted(v)[math.ceil(.95*len(v))-1],'max':max(v)} if v else None
result={'hardware':read('hardware.json'),'datasets':{},'runs':{},'shipping':'NOT_RUN','second_machine':'NOT_RUN','independent_character_review':'NOT_RUN'}
result['current_source_diff_sha256']=read('build-manifest.json')['source_diff_sha256']
for mode in ['dev','exposed-heldout','heldout','cpu']:
    d=read(mode+'-results.json')
    if not d:continue
    cases=d['cases'];supported=[x for x in cases if x['expected'] in actions];complete=[x for x in supported if len(x['turns'])==1];conditioned=[x for x in supported if len(x['turns'])>1]
    false_refusals=[x for x in complete if x['turn_results'][-1]['intent'] in ['refuse','clarify']]
    turns=[t for x in cases for t in x['turn_results']];latency=[t['seconds'] for t in turns]
    manifest=read(mode+'-manifest.json') or {};vram=[int(s['gpu'].split(',')[1].strip()) for s in manifest.get('gpu_samples',[]) if ',' in s['gpu']]
    grounded=[]
    for x in cases:
        t=x['turn_results'][-1];ctx=json.loads(t['context'])
        if x['expected']=='inventory':
            # This frozen corpus asks inventory questions about wood; check the authoritative numeric phrase.
            obs=ctx.get('last_seen_camp',{}).get('counts',{});m=re.search(r'有\s*(\d+)\s*份木材',t['line']) or re.search(r'木材\s*[是为有：]\s*(\d+)\s*份',t['line'])
            grounded.append(bool(m) and int(m.group(1))==obs.get('wood'))
        elif x['expected']=='recall':
            records=ctx.get('player_records',[])
            grounded.append(any(r['text'] in t['line'] and r['id'][:8].lower() in t['line'].lower() for r in records) or ('没有找到' in t['line'] and not records))
    result['datasets'][mode]={'overall':ratio(cases),'model_raw_understanding':ratio(cases,'model_understanding_pass'),'normalized_intent':ratio(cases,'semantic_pass'),'supported_actual':ratio(supported),'complete_supported_actual':ratio(complete),'conditioned_supported_actual':ratio(conditioned),'false_refusals':len(false_refusals),'false_refusal_denominator':len(complete),'false_refusal_rate':len(false_refusals)/len(complete) if complete else None,'incorrect_actionable_cards':sum(x['expected']=='safe' and x['turn_results'][-1]['intent']=='proposal' for x in cases),'unconfirmed_side_effects':sum(not x['unconfirmed_unchanged'] for x in cases),'grounded_readonly':{'pass':sum(grounded),'total':len(grounded)},'failures':[x['id'] for x in cases if not x['pass']],'model_failures':[x['id'] for x in cases if not x['model_understanding_pass']],'cold_first_seconds':latency[0] if latency else None,'warm_seconds':distribution(latency[1:]),'tokens_in':distribution([t['tokens_in'] for t in turns]),'tokens_out':distribution([t['tokens_out'] for t in turns]),'submit_api_seconds':distribution([t['submit_response_seconds'] for t in turns]),'confirm_api_seconds':distribution([x['actual']['confirm_response_seconds'] for x in cases if x.get('actual') and 'confirm_response_seconds' in x['actual']]),'frame_ms':d.get('frame_ms'),'device_vram_peak_mib':max(vram) if vram else None}
for mode in ['native','timeout','workshop','faults','lifecycle','interactions','playercraft','playerrepair','regression','physical','physicalb']:
    d=read(mode+'-results.json')
    if d is None:result['runs'][mode]='NOT_RUN';continue
    if mode=='native':
        idx=read('native-index.json') or {};result['runs'][mode]={k:idx.get(k) for k in ['succeeded','failed','notRun']};continue
    result['runs'][mode]={'passed':d['passed'],'checks':len(d.get('checks',{})),'failed_checks':[k for k,v in d.get('checks',{}).items() if not v],'model_cases':len(d.get('cases',d.get('model_trace',[]))),'error':d.get('error')}
for group in ['datasets','runs']:
    for mode,entry in result[group].items():
        if isinstance(entry,dict):
            manifest=read(mode+'-manifest.json') or {}
            entry['source_diff_sha256']=manifest.get('source_diff_sha256')
            entry['matches_current_source']=entry['source_diff_sha256']==result['current_source_diff_sha256']
            entry['started_utc']=manifest.get('started_utc')
base=read('baseline-results.json');dev=read('dev-results.json')
if base and dev:
    ids={x['id'] for x in base['cases']};common=[x for x in dev['cases'] if x['id'] in ids]
    result['baseline_common']={'baseline_sha':base['code_sha'],'baseline':ratio(base['cases']),'v2':ratio(common),'baseline_raw':ratio(base['cases'],'model_understanding_pass'),'v2_raw':ratio(common,'model_understanding_pass'),'scope':base['scope'],'new_B':ratio([x for x in dev['cases'] if x['expected'] in ['craft','craft_rope','repair']])}
h=result['datasets'].get('heldout')
if h:
    result['heldout_thresholds']={
        'complete_supported_at_least_95_percent':h['complete_supported_actual']['rate'] is not None and h['complete_supported_actual']['rate']>=.95,
        'conditioned_supported_at_least_90_percent':h['conditioned_supported_actual']['rate'] is not None and h['conditioned_supported_actual']['rate']>=.9,
        'false_refusal_at_most_5_percent':h['false_refusal_rate'] is not None and h['false_refusal_rate']<=.05,
        'zero_unconfirmed_effects':h['unconfirmed_side_effects']==0,
        'zero_incorrect_actionable_negative_cards':h['incorrect_actionable_cards']==0,
        'grounded_readonly':h['grounded_readonly']['total']>0 and h['grounded_readonly']['pass']==h['grounded_readonly']['total']}
result['cancel_api']=read('cancel-response.json')
result['http_timeout']=read('http-timeout-seconds.json')
(ev/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k not in ['hardware','datasets']},ensure_ascii=False,indent=2))
