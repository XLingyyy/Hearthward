"""Official Tripo v3 motion source adapter with resumable task journals.

API keys and signed URLs exist only in memory. Never retry a paid submission
whose result is ambiguous. Existing task IDs avoid rebilling on resume.
"""
from pathlib import Path
import urllib.request,urllib.error,json,time,hashlib,sys
from concurrent.futures import ThreadPoolExecutor

ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
KEYFILE=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\主角.tripo_api_key')
BASE='https://openapi.tripo3d.ai/v3'
PRESET={'quadruped':'preset:quadruped:walk','aquatic':'preset:aquatic:march','serpentine':'preset:serpentine:march'}

def main():
    key=KEYFILE.read_text(encoding='utf-8-sig').strip()
    def request(method,path,body=None):
        encoded=None if body is None else json.dumps(body).encode()
        req=urllib.request.Request(BASE+path,data=encoded,method=method,headers={'Authorization':'Bearer '+key,'Content-Type':'application/json'})
        try:
            response=json.load(urllib.request.urlopen(req,timeout=90))
        except urllib.error.HTTPError as e:
            response=json.loads(e.read().decode())
            raise RuntimeError('Tripo HTTP '+str(e.code)+': '+str(response.get('message',response.get('error',response.get('code')))))
        if response.get('code')!=0:raise RuntimeError('Tripo code '+str(response.get('code'))+': '+str(response.get('message')))
        return response['data']
    def run(job):
        if job['rig_type'] not in PRESET:return
        out=Path(job['output']);journal=out/'cloud_source.json'
        state=json.loads(journal.read_text(encoding='utf-8')) if journal.is_file() else {'provider':'Tripo official v3','animation':PRESET[job['rig_type']],'model':'v2.5-20260210','rig_task_id':job['rig_task_id'],'source_task_id':job['source_task_id'],'state':'new'}
        def save():journal.write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf-8')
        def poll(task):
            for _ in range(180):
                data=request('GET','/tasks/'+task)
                if data['status']=='success':return data
                if data['status'] in ('failed','cancelled'):raise RuntimeError('Task failed: '+str(data.get('error_code'))+' '+str(data.get('error_message'))[:300])
                time.sleep(5)
            raise RuntimeError('Task pending; resume from journal')
        try:
            artifact=out/'cloud_motion.fbx'
            if artifact.is_file() and state.get('state')=='downloaded':return
            if not state.get('animation_task_id'):
                try:
                    task=request('POST','/animations/retarget',{'input':state['rig_task_id'],'animation':state['animation'],'out_format':'fbx','animate_in_place':True,'export_with_geometry':True})
                except RuntimeError as e:
                    # Fresh rig only for an explicit expired/invalid input response.
                    if not any(s in str(e).lower() for s in ['expired','not found','invalid task','24 hours']):raise
                    rig=request('POST','/animations/rig',{'input':job['source_task_id'],'model':'v2.5-20260210','rig_type':job['rig_type'],'spec':'tripo','out_format':'fbx'})
                    state['rig_task_id']=rig['task_id'];state['state']='rig_queued';save()
                    rigdata=poll(rig['task_id']);state['rig_credits']=rigdata.get('credits_consumed');save()
                    task=request('POST','/animations/retarget',{'input':state['rig_task_id'],'animation':state['animation'],'out_format':'fbx','animate_in_place':True,'export_with_geometry':True})
                state['animation_task_id']=task['task_id'];state['state']='queued';save()
                print(job['slug']+' animation submitted '+task['task_id'],flush=True)
            data=poll(state['animation_task_id'])
            state['state']='success';state['animation_credits']=data.get('credits_consumed');save()
            url=data['output']['model_url']
            with urllib.request.urlopen(url,timeout=120) as response, artifact.open('wb') as stream:
                while chunk:=response.read(4*1024*1024):stream.write(chunk)
            state.update(state='downloaded',sha256=hashlib.sha256(artifact.read_bytes()).hexdigest(),bytes=artifact.stat().st_size)
            save();print(job['slug']+' source downloaded; credits '+str(state['animation_credits']),flush=True)
        except Exception as e:
            state['state']='error';state['error']=str(e)[:500];save();print(job['slug']+': '+str(e)[:500],flush=True)
    jobs=json.loads((ROOT/'jobs.json').read_text(encoding='utf-8'))
    if len(sys.argv)>1:jobs=[j for j in jobs if j['slug'] in sys.argv[1:]]
    with ThreadPoolExecutor(max_workers=3) as pool:list(pool.map(run,jobs))

if __name__=='__main__':main()
