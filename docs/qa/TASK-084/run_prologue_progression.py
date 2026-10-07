"""Root-run Development/API prologue probe; real movement/actions, own fresh profile.

Stops at the first path, geometry, safety or transaction failure. This is not OS
input evidence and never supplies first-rescue, Shipping or six-scene credit.
"""
import argparse
from http.client import BadStatusLine
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import re
import socket
import subprocess
import sys
import time
from urllib import error, request
import uuid


def vector(value):
    if not isinstance(value, dict):
        raise RuntimeError('Actual FVector object missing')
    result = {axis: float(value.get(axis, value.get(axis.lower(), math.nan))) for axis in 'XYZ'}
    if not all(math.isfinite(v) for v in result.values()):
        raise RuntimeError('Actual FVector has missing or non-finite coordinates')
    return result


def distance(a, b):
    return math.sqrt(sum((a[k]-b[k])**2 for k in 'XYZ'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, default=Path(__file__).resolve().parents[3]/'Hearthward.uproject')
    parser.add_argument('--ue-root', type=Path, default=Path('G:/UnrealEngine/UE_5.8'))
    parser.add_argument('--run', required=True)
    parser.add_argument('--build-evidence', type=Path, required=True,
                        help='Actual successful Editor build result; no auto-build is performed')
    parser.add_argument('--startup-timeout', type=int, default=180)
    parser.add_argument('--route-timeout', type=int, default=240)
    parser.add_argument('--input-mode', choices=('navigation','tick','http'), default='navigation',
                        help='navigation uses normal engine path following; tick is refused by current RC security policy')
    parser.add_argument('--bounded-final-navigation', action='store_true',
                        help='Opt in after the actual courtyard node: project bounded QA query candidates, then follow only complete generated nav paths')
    parser.add_argument('--final-query-extent-xy', type=float, default=None,
                        help='Predeclare one fixed X/Y nav query extent in cm for bounded final navigation (default 100; range 100..1000; Z remains 220)')
    parser.add_argument('--diagnose-final-surface', action='store_true',
                        help='On failed bounded projection, record only fixed vertical collision traces and one Z1000 nav projection; never move from diagnostics')
    parser.add_argument('--grounded-final-navigation', action='store_true',
                        help='For bounded final navigation, require actual Landscape/NatureGround trace and query at ImpactPoint plus 100cm')
    parser.add_argument('--prefer-complete-final-path', action='store_true',
                        help='Before each bounded query, use the formal target only when its actual nav projection has a complete path')
    args = parser.parse_args()
    if args.prefer_complete_final_path and not args.bounded_final_navigation:
        parser.error('complete final path preference requires bounded final navigation')
    if args.grounded_final_navigation and not args.bounded_final_navigation:
        parser.error('grounded final navigation requires bounded final navigation')
    if args.diagnose_final_surface and not args.bounded_final_navigation:
        parser.error('final surface diagnostics require bounded final navigation')
    if args.final_query_extent_xy is not None and not args.bounded_final_navigation:
        parser.error('final query extent is only valid with bounded final navigation')
    args.final_query_extent_xy=100 if args.final_query_extent_xy is None else args.final_query_extent_xy
    if not 100<=args.final_query_extent_xy<=1000:
        parser.error('final query X/Y extent must be finite and within 100..1000cm')
    if args.bounded_final_navigation and args.input_mode!='navigation':
        parser.error('bounded final navigation requires normal engine PathFollowing mode')
    if args.input_mode=='tick':
        parser.error('tick mode is unsupported by current remote-Python security policy; use navigation or historical http')
    if Path(args.run).name != args.run or args.run in ('', '.', '..'):
        parser.error('--run must be one new directory name')
    if not 0 < args.startup_timeout <= 180 or not 0 < args.route_timeout <= 300:
        parser.error('Startup must be 1..180s and route must be 1..300s')
    project = args.project.resolve()
    if not project.is_file():
        parser.error('Project is missing')
    build_evidence = args.build_evidence.resolve()
    if not build_evidence.is_file():
        parser.error('Actual build evidence is missing')
    build_result = json.loads(build_evidence.read_text(encoding='utf-8-sig'))
    if not isinstance(build_result,dict) or build_result.get('ok') is not True:
        parser.error('Actual build evidence must record ok:true')
    build_payload = build_result.get('payload',{})
    if build_result.get('operation') != 'build.project' or any(
        build_payload.get(k) != v for k,v in {
            'target':'HearthwardEditor','platform':'Win64','configuration':'Development',
            'dry_run':False,'returncode':0}.items()):
        parser.error('Evidence must bind an actual successful HearthwardEditor Win64 Development build')
    binary = project.parent/'Binaries/Win64/UnrealEditor-Hearthward.dll'
    if not binary.is_file():
        parser.error('Built Editor game module is missing')
    factory = next((p for p in project.parent.parents if (p/'engine_adapters/ue5/__init__.py').is_file()), None)
    if factory is None:
        parser.error('Prepared GameFactory public UEClient is missing')
    sys.path.insert(0, str(factory))
    from engine_adapters.ue5 import UEClient
    out = project.parent/'.agent-local/qa/TASK-084'/args.run
    out.mkdir(parents=True, exist_ok=False)
    pool = str(uuid.uuid4())
    profile, log = out/'profile', out/'runtime.log'
    with socket.socket() as probe:
        probe.bind(('127.0.0.1', 0))
        port = probe.getsockname()[1]
    ue = UEClient(project_path=project, ue_root=args.ue_root, host='127.0.0.1', port=port)
    opener = request.build_opener(request.ProxyHandler({}))
    report = {'task':'TASK-084', 'run':args.run, 'created_utc':datetime.now(timezone.utc).isoformat(),
              'status':'NOT_RUN', 'stage':'launch', 'pool':pool, 'profile':str(profile),
              'scope':'fresh_new_game_artifact_follow_escape_safe_save',
              'evidence_level':'Development standalone -game; public RC movement/actions; NOT_OS_INPUT',
              'shipping':'NOT_RUN', 'first_rescue':'NOT_RUN', 'six_scene_performance':'NOT_RUN',
               'qa_actor_position_writes':False, 'qa_velocity_or_movement_mode_writes':False,
               'qa_gift_or_raw_progress_writes':False, 'qa_forced_tick_or_clock_writes':False,
              'model_submission':'NOT_RUN', 'observations':[], 'screenshots_requested':[]}
    report['input_mode']=args.input_mode
    report['remote_python']='UNSUPPORTED_BY_CURRENT_SECURITY_POLICY_NOT_REQUESTED'
    report['movement_api']='SimpleMoveToLocation / normal engine PathFollowing' if args.input_mode=='navigation' else 'historical one-shot AddMovementInput'
    report['normal_os_input']='NOT_RUN'
    report['final_navigation']={'bounded_opt_in':args.bounded_final_navigation,
        'prefer_complete_formal_goal':args.prefer_complete_final_path,
        'formal_goal_step_limit':'No artificial segment limit; actual complete path required',
        'mode':'bounded_projected_nav_segments' if args.bounded_final_navigation else 'strict_full_path',
        'candidate_max_step_cm':2000 if args.bounded_final_navigation else None,
        'candidate_role':'QA query only; actual movement goal requires GeneratedNav projection and complete paths',
        'query_extent':{'X':args.final_query_extent_xy,'Y':args.final_query_extent_xy,'Z':220},
        'query_extent_policy':'fixed before run; no failure-driven expansion','game_route_nodes_added':False,
        'evidence_level':'public RC / normal engine PathFollowing; NOT_OS_INPUT',
        'grounded_qa':{'enabled':args.grounded_final_navigation,
            'query_point_source':'actual confirmed Landscape/NatureGround ImpactPoint plus Z100',
            'source_or_invoker_changes':False,'diagnostic_wide_z_point_used':False},
        'surface_diagnostics':{'enabled':args.diagnose_final_surface,'scope':'READ_ONLY_NO_MOVEMENT',
            'vertical_trace_half_span_cm':4000,'diagnostic_nav_z_extent_cm':1000,
            'second_trace_policy':'same ray ignores Home only after exact class confirmation',
            'results_used_for_navigation':False}}
    report['binary_binding'] = {'build_result':str(build_evidence), 'path':str(binary),
        'size_bytes':binary.stat().st_size, 'mtime_ns':binary.stat().st_mtime_ns,
        'new_working_source_validation':'NOT_RUN', 'auto_build':False}
    pid = None
    deadline = time.monotonic()+args.startup_timeout
    last_process_check = 0
    context = None
    gameplay = '/Script/Engine.Default__GameplayStatics'
    system = '/Script/Engine.Default__KismetSystemLibrary'
    subsystems = '/Script/Engine.Default__SubsystemBlueprintLibrary'
    navigation = '/Script/NavigationSystem.Default__NavigationSystemV1'
    ai_navigation = '/Script/AIModule.Default__AIBlueprintHelperLibrary'
    rules = {
        '/Script/Engine.GameplayStatics':['GetPlayerPawn','GetPlayerController','GetCurrentLevelName','GetGameInstance','IsGamePaused'],
        '/Script/Engine.KismetSystemLibrary':['GetOuterObject','GetPathName','ExecuteConsoleCommand'],
        '/Script/Engine.SubsystemBlueprintLibrary':['GetGameInstanceSubsystem','GetWorldSubsystem'],
        '/Script/Hearthward.HearthwardCharacter':['GetLevel','GetComponentByClass','K2_GetActorLocation','AddMovementInput','IsMoveInputIgnored'],
        '/Script/Engine.PlayerController':['GetHUD','StopMovement'],
        '/Script/Hearthward.HearthwardScreenWidget':['GetPage','ExecuteAction','DescribeLayout','DescribeQuestGuidance'],
        '/Script/Hearthward.HearthwardLoadingSubsystem':['IsLoading'],
        '/Script/Hearthward.HearthwardCampaignSubsystem':['Describe','Busy','Interact','Prompt'],
        '/Script/Hearthward.HearthwardGameplayComponent':['InCombat','OrderCompanion'],
        '/Script/Hearthward.HearthwardCombatComponent':['Busy','MovementLocked'],
        '/Script/Hearthward.HearthwardTimedActionComponent':['GetStatus'],
        '/Script/Hearthward.HearthwardSaveSubsystem':['GetPoints','GetStatus','GetCampaignId'],
        '/Script/NavigationSystem.NavigationSystemV1':['FindPathToLocationSynchronously','GetNavigationSystem',
            'IsNavigationBeingBuiltOrLocked','K2_ProjectPointToNavigation'],
        '/Script/NavigationSystem.NavigationPath':['IsValid','IsPartial'],
        '/Script/AIModule.AIBlueprintHelperLibrary':['SimpleMoveToLocation','GetCurrentPath'],
    }
    if args.diagnose_final_surface or args.grounded_final_navigation:
        rules['/Script/Engine.KismetSystemLibrary'].append('LineTraceSingle')
    policy = out/'RemoteControl.ini'
    lines = ['[/Script/RemoteControlCommon.RemoteControlSettings]', 'bAllowAnyRemoteFunctionCall=False',
             'bAutoStartWebSocketServer=False', 'bAllowConsoleCommandRemoteExecution=True',
             'RemoteControlHttpServerPort='+str(port)]
    lines += ['CustomAllowedRemoteFunctionCalls=(ClassPath="'+cls+'",FunctionName=("'+fn+'"),bAllowChildClasses=False)'
              for cls, functions in rules.items() for fn in functions]
    policy.write_text('\n'.join(lines)+'\n', encoding='utf-8')
    report['remote_policy'] = {'allow_any':False, 'exact_rules':rules, 'file':str(policy)}

    def save_report():
        (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n', encoding='utf-8')

    def process_alive():
        result = subprocess.run(['powershell','-NoProfile','-Command',
            '$p=Get-Process -Id '+str(pid)+' -ErrorAction SilentlyContinue; if($p){"ALIVE"}else{"EXITED"}'],
            capture_output=True,text=True,encoding='utf-8',timeout=5)
        if result.returncode or result.stdout.strip() not in ('ALIVE','EXITED'):
            raise RuntimeError('Owned UE PID could not be checked')
        return result.stdout.strip() == 'ALIVE'

    def guard():
        nonlocal last_process_check
        if time.monotonic() >= deadline:
            raise TimeoutError('Bounded session expired at '+report['stage'])
        if pid and time.monotonic()-last_process_check >= 1:
            last_process_check = time.monotonic()
            if log.is_file() and any(mark in log.read_text(encoding='utf-8-sig') for mark in ('=== Critical error:', 'Fatal error!', 'Unhandled Exception:')):
                raise RuntimeError('Owned UE fatal/critical startup/runtime error')
            if not process_alive():
                raise RuntimeError('Owned UE PID exited')

    def http(name, route, body=None, retain=True):
        guard()
        event = {'name':name,'stage':report['stage'],'at_monotonic':time.monotonic(),'request':body}
        try:
            req = request.Request('http://127.0.0.1:'+str(port)+route,
                data=None if body is None else json.dumps(body).encode('utf-8'),
                headers={'Content-Type':'application/json'},method='GET' if body is None else 'PUT')
            with opener.open(req,timeout=min(10,max(.1,deadline-time.monotonic()))) as response:
                event['http_status'] = response.status
                value = json.loads(response.read().decode('utf-8-sig'))
            if not isinstance(value,dict):
                raise RuntimeError('RC JSON object missing at '+name)
            event['response'] = value if retain else {'retained':False,'type':'object'}
            return value
        except error.HTTPError as http_error:
            event['http_status']=http_error.code
            raw=http_error.read(16385)
            event['http_error_body_bytes_read']=len(raw)
            event['http_error_body_truncated']=len(raw)>16384
            try:
                body=raw[:16384].decode('utf-8-sig')
                body=re.sub(r'(?i)(authorization|api[_-]?key|secret[_-]?key|access[_-]?token|password)(["\s:=]+)(?:Bearer\s+)?([^\s",;}]+)',
                            r'\1\2[REDACTED]',body)
                body=re.sub(r'\b(?:sk-[A-Za-z0-9_-]{20,}|ghp_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,})\b',
                            '[REDACTED]',body)
                event['http_error_body_utf8_redacted']=body
            except UnicodeDecodeError:
                event['http_error_body_state']='NOT_READ_UTF8'
            event['error']=str(http_error)
            raise
        except Exception as exc:
            event['error'] = str(exc)
            raise
        finally:
            with (out/'http-events.jsonl').open('a',encoding='utf-8') as stream:
                stream.write(json.dumps(event,ensure_ascii=False)+'\n')

    def call(obj, fn, params=None, retain=True):
        return http(fn,'/remote/object/call',{'objectPath':obj,'functionName':fn,
            'parameters':params or {},'generateTransaction':False},retain).get('ReturnValue')

    def ref(value):
        if not isinstance(value,str) or not value.startswith('/'):
            raise RuntimeError('Actual UObject reference is missing')
        return value

    def prop(obj, name):
        return http('read.'+name,'/remote/object/property',{'objectPath':obj,'propertyName':name,'access':'READ_ACCESS'}).get(name)

    def references(seed):
        pawn = ref(call(gameplay,'GetPlayerPawn',{'WorldContextObject':seed,'PlayerIndex':0}))
        level = ref(call(pawn,'GetLevel'))
        world = ref(call(system,'GetOuterObject',{'Object':level}))
        pc = ref(call(gameplay,'GetPlayerController',{'WorldContextObject':pawn,'PlayerIndex':0}))
        hud = ref(call(pc,'GetHUD'))
        screen = ref(prop(hud,'Screen'))
        return {'pawn':pawn,'world':world,'screen':screen,'controller':pc,
                'level':call(gameplay,'GetCurrentLevelName',{'WorldContextObject':pawn,'bRemovePrefixString':True})}

    def subsystem(cls, instance=False):
        return ref(call(subsystems,'GetGameInstanceSubsystem' if instance else 'GetWorldSubsystem',
            {'ContextObject':context['pawn'],'Class':'/Script/Hearthward.'+cls}))

    def screenshot(label):
        call(system,'ExecuteConsoleCommand',{'WorldContextObject':context['pawn'],'Command':'Shot SHOWUI','SpecificPlayer':None})
        report['screenshots_requested'].append({'label':label,'at_monotonic':time.monotonic(),
            'location':'owned profile Saved/Screenshots; actual output existence is checked separately by root'})

    def wait_loading(loading):
        while True:
            busy = call(loading,'IsLoading')
            if not isinstance(busy,bool):
                raise RuntimeError('Actual loading getter did not return a Boolean')
            if not busy:
                return
            time.sleep(.2)

    def campaign():
        state = json.loads(call(camp,'Describe',retain=False))
        if not isinstance(state,dict):
            raise RuntimeError('Actual Campaign Describe JSON object missing')
        return state

    def campaign_busy():
        value = call(camp,'Busy')
        if not isinstance(value,bool):
            raise RuntimeError('Actual Campaign Busy getter did not return a Boolean')
        return value

    def safe_input():
        if call(gameplay,'IsGamePaused',{'WorldContextObject':context['pawn']}) is not False:
            raise RuntimeError('Paused world cannot be driven')
        if call(context['screen'],'GetPage') != 'hud' or call(context['pawn'],'IsMoveInputIgnored') is not False:
            raise RuntimeError('HUD/controller movement gate refused ordinary input')
        health = prop(player_gameplay,'Health')
        life = prop(survival,'State')
        if not isinstance(health,(float,int)) or health <= 0 or not isinstance(life,dict) or str(life.get('Life',life.get('life',''))).split('::')[-1] != 'Alive':
            raise RuntimeError('Player is not actually alive')
        for obj,fn in ((player_gameplay,'InCombat'),(combat,'Busy'),(combat,'MovementLocked')):
            value = call(obj,fn)
            if not isinstance(value,bool):
                raise RuntimeError('Actual '+fn+' getter did not return a Boolean')
            if value:
                raise RuntimeError('Combat action/movement guard is active: '+fn)
        action_status = str(call(timed,'GetStatus')).split('::')[-1]
        if action_status not in ('Idle','Running','Interrupted','Completed'):
            raise RuntimeError('Actual timed-action status is unknown')
        if action_status == 'Running':
            raise RuntimeError('A real timed action is active; no bypass or forced cancellation')

    def position():
        return vector(call(context['pawn'],'K2_GetActorLocation'))

    def stop_navigation():
        nonlocal deadline
        deadline=max(deadline,time.monotonic()+3)
        call(context['controller'],'StopMovement')

    def walk_to(goal, arrival_cm=210):
        try:
            walk_path(goal, arrival_cm)
        finally:
            if args.input_mode=='navigation':
                had_error=sys.exc_info()[0] is not None
                try:
                    stop_navigation()
                except Exception as stop_error:
                    report.setdefault('navigation_stop_errors',[]).append(str(stop_error))
                    if not had_error:
                        raise

    def walk_path(goal, arrival_cm=210):
        nonlocal deadline
        current = position()
        if distance(current,goal)<arrival_cm:
            return
        if report['stage']=='actual_escape_walk':
            started=time.monotonic()
            previous_deadline=deadline
            readiness_deadline=started+30
            deadline=min(previous_deadline,readiness_deadline)
            readiness={'stage':report['stage'],'state':'WAITING','actual_goal':dict(goal),
                'started_monotonic':started,'max_wait_seconds':30,'poll_sleep_seconds':.5,'samples':[]}
            report['nav_readiness']=readiness
            report.setdefault('nav_readiness_history',[]).append(readiness)
            try:
                readiness['navigation_system']=ref(call(navigation,'GetNavigationSystem',
                    {'WorldContextObject':context['pawn']}))
                while True:
                    guard();safe_input()
                    busy=call(navigation,'IsNavigationBeingBuiltOrLocked',
                        {'WorldContextObject':context['pawn']})
                    sampled=time.monotonic()
                    if not isinstance(busy,bool):
                        raise RuntimeError('Navigation readiness getter did not return a Boolean')
                    readiness['samples'].append({'at_monotonic':sampled,'building_or_locked':busy})
                    if sampled>=readiness_deadline:
                        readiness['state']='READINESS_TIMEOUT'
                        raise RuntimeError('Navigation readiness remained unverified within 30s; no path fallback')
                    if busy is False:
                        readiness['state']='READY'
                        break
                    time.sleep(min(.5,max(0,deadline-time.monotonic())))
            except Exception as readiness_error:
                if time.monotonic()>=readiness_deadline and previous_deadline>=readiness_deadline:
                    readiness['state']='READINESS_TIMEOUT'
                    readiness['error']=str(readiness_error)
                    raise RuntimeError('Navigation readiness remained unverified within 30s; no path fallback') from readiness_error
                readiness['state']='NOT_READ'
                readiness['error']=str(readiness_error)
                raise
            finally:
                readiness['ended_monotonic']=time.monotonic()
                readiness['actual_wait_seconds']=readiness['ended_monotonic']-started
                deadline=previous_deadline
            current=position()
        path = ref(call(navigation,'FindPathToLocationSynchronously',{'WorldContextObject':context['pawn'],
            'PathStart':current,'PathEnd':goal,'PathfindingContext':context['pawn'],'FilterClass':None}))
        path_valid=call(path,'IsValid')
        partial_read_error=None
        try:
            path_partial=call(path,'IsPartial')
        except Exception as partial_error:
            if path_valid is True:
                raise
            path_partial=None
            partial_read_error=str(partial_error)
        if path_valid is not True or path_partial is not False:
            diagnostic={'stage':report['stage'],'state':'READ_COMPLETE','path':path,
                'actual_start':current,'actual_goal':goal,'path_is_valid':path_valid,
                'path_is_partial':path_partial,'path_points':'NOT_READ',
                'query_extent':dict(report['final_navigation']['query_extent'])
                    if args.bounded_final_navigation and report.get('guidance_state')=='WALKING_ACTUAL_FINAL_TARGET'
                    else {'X':100,'Y':100,'Z':220},
                'purpose':'READ_ONLY_NO_FALLBACK'}
            report.setdefault('nav_diagnostics',[]).append(diagnostic)
            if partial_read_error is not None:
                diagnostic['partial_read']={'state':'NOT_READ','error':partial_read_error}
                diagnostic['state']='READ_PARTIAL_WITH_NOT_READ'
            queries=[('navigation_system','GetNavigationSystem',{'WorldContextObject':context['pawn']}),
                ('building_or_locked','IsNavigationBeingBuiltOrLocked',{'WorldContextObject':context['pawn']}),
                ('start_projection','K2_ProjectPointToNavigation',{'WorldContextObject':context['pawn'],
                    'Point':current,'NavData':None,'FilterClass':None,'QueryExtent':diagnostic['query_extent']}),
                ('goal_projection','K2_ProjectPointToNavigation',{'WorldContextObject':context['pawn'],
                    'Point':goal,'NavData':None,'FilterClass':None,'QueryExtent':diagnostic['query_extent']})]
            for label,fn,params in queries:
                try:
                    answer=http(fn,'/remote/object/call',{'objectPath':navigation,'functionName':fn,
                        'parameters':params,'generateTransaction':False})
                    value=answer.get('ReturnValue')
                    if label=='navigation_system':
                        ref(value)
                    elif not isinstance(value,bool):
                        raise RuntimeError('Navigation diagnostic did not return a Boolean: '+fn)
                    entry={'state':'READ','response':answer}
                    if label.endswith('_projection'):
                        entry['projected_location_valid']=value
                        if value:
                            vector(answer.get('ProjectedLocation'))
                    diagnostic[label]=entry
                except Exception as diagnostic_error:
                    diagnostic[label]={'state':'NOT_READ','error':str(diagnostic_error)}
                    diagnostic['state']='READ_PARTIAL_WITH_NOT_READ'
            raise RuntimeError('Actual NavPath is invalid or partial; no guessed geometry fallback')
        points = prop(path,'PathPoints')
        if not isinstance(points,list) or len(points)<2:
            raise RuntimeError('Actual complete NavPath has no usable points')
        points = [vector(p) for p in points]
        report['observations'].append({'stage':report['stage'],'nav_path':points,'goal':goal,'started_position':current})
        if args.input_mode=='navigation':
            safe_input()
            call(ai_navigation,'SimpleMoveToLocation',{'Controller':context['controller'],'Goal':goal})
            actual_path=ref(call(ai_navigation,'GetCurrentPath',{'Controller':context['controller']}))
            if call(actual_path,'IsValid') is not True or call(actual_path,'IsPartial') is not False:
                raise RuntimeError('Actual controller path after SimpleMove is invalid or partial')
            actual_points=prop(actual_path,'PathPoints')
            if not isinstance(actual_points,list) or len(actual_points)<2:
                raise RuntimeError('Actual controller path after SimpleMove has no usable points')
            report['observations'].append({'stage':report['stage'],'controller_actual_nav_path':[vector(p) for p in actual_points],
                'goal':goal,'movement':'engine PathFollowing; may use RequestDirectMove internally; no QA velocity setter'})
            last_progress,last_position,last_sample=time.monotonic(),current,0
            while True:
                guard();safe_input();current=position()
                if time.monotonic()-last_sample>=1:
                    report['observations'].append({'stage':report['stage'],'actual_position':current,
                        'distance_to_goal_cm':distance(current,goal),'at_monotonic':time.monotonic()})
                    last_sample=time.monotonic()
                if distance(current,goal)<arrival_cm:
                    return
                if distance(current,last_position)>20:
                    last_progress,last_position=time.monotonic(),current
                if time.monotonic()-last_progress>8:
                    report['blocked_position']=current
                    raise RuntimeError('Actual navigation made no progress for 8s; no guessed route or position override')
                time.sleep(.1)
        last_progress, last_position, last_safety = time.monotonic(), current, 0
        for target in points[1:]:
            while True:
                guard()
                if time.monotonic()-last_safety>.25:
                    safe_input();last_safety=time.monotonic()
                current = position()
                if distance(current,goal)<arrival_cm:
                    return
                delta = {k:target[k]-current[k] for k in 'XYZ'}
                horizontal = math.hypot(delta['X'],delta['Y'])
                if horizontal<65:
                    if abs(delta['Z'])>150:
                        raise RuntimeError('Reached waypoint XY but not its real elevation')
                    break
                if distance(current,last_position)>20:
                    last_progress,last_position = time.monotonic(),current
                if time.monotonic()-last_progress>8:
                    report['blocked_position'] = current
                    raise RuntimeError('Actual movement made no progress for 8s; input/collision cause remains unproven')
                direction={'X':delta['X']/horizontal,'Y':delta['Y']/horizontal,'Z':0}
                call(context['pawn'],'AddMovementInput',{'WorldDirection':direction,'ScaleValue':1,'bForce':False})
                time.sleep(.025)
        if distance(position(),goal)>=arrival_cm:
            raise RuntimeError('Actual NavPath finished outside interaction distance')

    def read_final_surface(candidate):
        diagnostic={'scope':'READ_ONLY_NO_MOVEMENT','state':'READ_ONLY_COMPLETE',
                    'trace_start':dict(candidate,Z=candidate['Z']+4000),
                    'trace_end':dict(candidate,Z=candidate['Z']-4000),'traces':[]}
        home_owner=None
        for trace_index in range(2):
            if trace_index==1 and home_owner is None:
                break
            ignored=[context['pawn']]+([home_owner] if trace_index==1 else [])
            trace={'state':'NOT_READ','kind':'first_blocking_collision' if trace_index==0 else 'same_ray_ignore_confirmed_home',
                   'actors_to_ignore':ignored}
            diagnostic['traces'].append(trace)
            try:
                guard();safe_input()
                answer=http('LineTraceSingle','/remote/object/call',
                    {'objectPath':system,'functionName':'LineTraceSingle',
                     'parameters':{'WorldContextObject':context['pawn'],
                         'Start':diagnostic['trace_start'],'End':diagnostic['trace_end'],
                         'TraceChannel':'TraceTypeQuery1','bTraceComplex':False,
                         'ActorsToIgnore':ignored,'DrawDebugType':'None','bIgnoreSelf':True,'DrawTime':0},
                     'generateTransaction':False})
                trace.update(state='READ',response=answer,surface_classification='UNCONFIRMED')
                if answer.get('ReturnValue') is not True:
                    trace['surface_classification']='NO_BLOCKING_HIT_READ'
                    continue
                hit=answer.get('OutHit')
                if not isinstance(hit,dict):
                    raise RuntimeError('Actual LineTraceSingle OutHit object missing')
                trace['impact_point']=vector(hit.get('ImpactPoint'))
                component=ref(hit.get('Component'))
                trace['component']=component
                trace['component_tags']=prop(component,'ComponentTags')
                actor=ref(call(system,'GetOuterObject',{'Object':component}))
                description=http('describe.trace_actor','/remote/object/describe',{'objectPath':actor})
                actor_class=ref(description.get('Class'))
                actor_tags=prop(actor,'Tags')
                trace.update(actor=actor,actor_class=actor_class,actor_tags=actor_tags,
                             actor_name=description.get('Name'))
                if actor_class in ('/Script/Landscape.Landscape','/Script/Landscape.LandscapeProxy',
                                   '/Script/Landscape.LandscapeStreamingProxy') or (
                                   isinstance(actor_tags,list) and 'Hearthward.NatureGround' in actor_tags):
                    trace['surface_classification']='CONFIRMED_GROUND_COLLISION'
                elif actor_class=='/Script/Hearthward.HearthwardHometownFortress':
                    trace['surface_classification']='CONFIRMED_FORTRESS_COLLISION'
                    if trace_index==0:
                        home_owner=actor
                else:
                    trace['surface_classification']='OTHER_COLLISION_SURFACE'
            except Exception as trace_error:
                trace.update(state='NOT_READ_OR_PARTIAL',error=str(trace_error))
                diagnostic['state']='READ_ONLY_PARTIAL'
        return diagnostic

    def walk_guided_escape():
        report['escape_goal']=vector(campaign()['positions']['prologue_exit'])
        report['guidance_api']='HearthwardScreenWidget.DescribeQuestGuidance / existing BlueprintPure'
        reached=None
        while True:
            guard();safe_input()
            guidance=json.loads(call(context['screen'],'DescribeQuestGuidance'))
            if not isinstance(guidance,dict):
                raise RuntimeError('Actual quest guidance JSON object missing')
            current=position()
            sample={'at_monotonic':time.monotonic(),'actual_position':current,'guidance':guidance}
            report.setdefault('guidance_nodes',[]).append(sample)
            if guidance.get('visible') is not True or guidance.get('location')!='prologue_exit':
                report['guidance_state']='TARGET_UNAVAILABLE_STOPPED'
                raise RuntimeError('Actual prologue exit guidance is unavailable; no target fallback')
            if guidance.get('route_visible') is not True:
                if not reached or reached.get('state')!='NODE_REACHED' or reached.get('route_id')!='courtyard_side_gate':
                    report['guidance_state']='ROUTE_UNAVAILABLE_STOPPED'
                    raise RuntimeError('Actual route_visible is false before the final spatial node was reached')
                coordinates=guidance.get('world')
                if not isinstance(coordinates,list) or len(coordinates)!=3:
                    raise RuntimeError('Actual guidance final target coordinates missing')
                goal=vector(dict(zip('XYZ',coordinates)))
                sample.update(state='WALKING_ACTUAL_FINAL_TARGET',world=goal,arrival_cm=210,
                              source='current DescribeQuestGuidance.world',
                              distance_to_target_cm=distance(current,goal))
                report['actual_guidance_final_target']=sample
                report['guidance_state']='WALKING_ACTUAL_FINAL_TARGET'
                save_report()
                if args.bounded_final_navigation:
                    previous_projected=None
                    while True:
                        guard();safe_input()
                        latest=json.loads(call(context['screen'],'DescribeQuestGuidance'))
                        if not isinstance(latest,dict) or latest.get('visible') is not True or latest.get('location')!='prologue_exit':
                            raise RuntimeError('Actual final target became unavailable during bounded navigation')
                        coordinates=latest.get('world')
                        if not isinstance(coordinates,list) or len(coordinates)!=3:
                            raise RuntimeError('Actual final target coordinates missing during bounded navigation')
                        if vector(dict(zip('XYZ',coordinates)))!=goal:
                            raise RuntimeError('Actual final target changed during bounded navigation')
                        current=position()
                        observed={'at_monotonic':time.monotonic(),'actual_position':current,'guidance':latest,
                                  'distance_to_final_target_cm':distance(current,goal)}
                        report.setdefault('final_guidance_observations',[]).append(observed)
                        span=distance(current,goal)
                        if span<210:
                            break
                        step=min(2000,span)
                        candidate={axis:current[axis]+(goal[axis]-current[axis])*step/span for axis in 'XYZ'}
                        segment={'state':'QUERYING_GENERATED_NAV','actual_start':current,'final_world':goal,
                                 'guidance':latest,'qa_query_candidate':candidate,'candidate_step_cm':step,
                                 'candidate_is_game_route_node':False,
                                 'query_extent':dict(report['final_navigation']['query_extent']),
                                 'nav_readiness_samples':[]}
                        report.setdefault('final_navigation_segments',[]).append(segment)
                        ready_started=time.monotonic()
                        while True:
                            guard();safe_input()
                            busy=call(navigation,'IsNavigationBeingBuiltOrLocked',{'WorldContextObject':context['pawn']})
                            if not isinstance(busy,bool):
                                raise RuntimeError('Bounded navigation readiness did not return a Boolean')
                            segment['nav_readiness_samples'].append({'at_monotonic':time.monotonic(),'building_or_locked':busy})
                            if time.monotonic()-ready_started>=30:
                                raise RuntimeError('Bounded navigation readiness was not established within 30s')
                            if not busy:
                                break
                            time.sleep(.5)
                        confirmed=json.loads(call(context['screen'],'DescribeQuestGuidance'))
                        if not isinstance(confirmed,dict) or confirmed.get('visible') is not True or confirmed.get('location')!='prologue_exit':
                            raise RuntimeError('Actual final target became unavailable while waiting for generated nav')
                        coordinates=confirmed.get('world')
                        if not isinstance(coordinates,list) or len(coordinates)!=3 or vector(dict(zip('XYZ',coordinates)))!=goal:
                            raise RuntimeError('Actual final target changed while waiting for generated nav')
                        current=position()
                        span=distance(current,goal)
                        if span<210:
                            segment['state']='FORMAL_TARGET_REACHED_BEFORE_QUERY'
                            break
                        if args.prefer_complete_final_path:
                            final_projection=http('K2_ProjectPointToNavigation.formal_goal','/remote/object/call',
                                {'objectPath':navigation,'functionName':'K2_ProjectPointToNavigation',
                                 'parameters':{'WorldContextObject':context['pawn'],'Point':goal,
                                               'NavData':None,'FilterClass':None,'QueryExtent':segment['query_extent']},
                                 'generateTransaction':False})
                            probe={'formal_world':goal,'projection':final_projection,'complete_path':False}
                            segment['formal_goal_probe']=probe
                            if final_projection.get('ReturnValue') is True:
                                final_nav_point=vector(final_projection.get('ProjectedLocation'))
                                final_path=ref(call(navigation,'FindPathToLocationSynchronously',
                                    {'WorldContextObject':context['pawn'],'PathStart':current,
                                     'PathEnd':final_nav_point,'PathfindingContext':context['pawn'],'FilterClass':None}))
                                probe.update(path=final_path,valid=call(final_path,'IsValid'),
                                             partial=call(final_path,'IsPartial'),projected_goal=final_nav_point)
                                probe['complete_path']=probe['valid'] is True and probe['partial'] is False
                                if probe['complete_path']:
                                    segment.update(state='WALKING_COMPLETE_FORMAL_PATH',actual_projected_goal=final_nav_point,
                                                   selected_goal_source='formal game target; complete generated nav path')
                                    save_report()
                                    walk_to(final_nav_point)
                                    arrived=position()
                                    segment.update(state='FORMAL_TARGET_REACHED',actual_arrival=arrived,
                                                   distance_at_arrival_cm=distance(arrived,goal))
                                    if distance(arrived,goal)>=210:
                                        raise RuntimeError('Complete nav path ended outside the formal target arrival radius')
                                    save_report()
                                    break
                            save_report()
                        step=min(2000,span)
                        candidate={axis:current[axis]+(goal[axis]-current[axis])*step/span for axis in 'XYZ'}
                        segment.update(actual_start=current,guidance_after_nav_ready=confirmed,
                                       qa_query_candidate=candidate,candidate_step_cm=step)
                        query_point=dict(candidate)
                        if args.grounded_final_navigation:
                            surface=read_final_surface(candidate)
                            segment['grounded_surface_read']=surface
                            hit=surface['traces'][-1] if surface['traces'] else {}
                            ground_class=hit.get('actor_class') in (
                                '/Script/Landscape.Landscape','/Script/Landscape.LandscapeStreamingProxy')
                            ground_tag=isinstance(hit.get('actor_tags'),list) and 'Hearthward.NatureGround' in hit['actor_tags']
                            if hit.get('state')!='READ' or not (ground_class or ground_tag):
                                raise RuntimeError('Bounded grounded navigation has no confirmed actual terrain surface; no fallback')
                            ground=vector(hit.get('impact_point'))
                            query_point=dict(ground,Z=ground['Z']+100)
                            segment.update(confirmed_ground=ground,navigation_query_point=query_point,
                                           query_point_source='confirmed actual terrain ImpactPoint plus Z100')
                        else:
                            segment['navigation_query_point']=query_point
                        save_report()
                        projected=http('K2_ProjectPointToNavigation','/remote/object/call',
                            {'objectPath':navigation,'functionName':'K2_ProjectPointToNavigation',
                             'parameters':{'WorldContextObject':context['pawn'],'Point':query_point,
                                           'NavData':None,'FilterClass':None,'QueryExtent':segment['query_extent']},
                             'generateTransaction':False})
                        segment['projection_response']=projected
                        if projected.get('ReturnValue') is not True:
                            if args.diagnose_final_surface:
                                diagnostic=read_final_surface(candidate)
                                diagnostic.update(original_candidate=dict(candidate),original_projection=projected,
                                    original_query_point=dict(query_point),
                                    diagnostic_query_extent={'X':args.final_query_extent_xy,
                                        'Y':args.final_query_extent_xy,'Z':1000},
                                    results_used_for_navigation=False)
                                segment['surface_diagnostic']=diagnostic
                                try:
                                    guard();safe_input()
                                    wide=http('K2_ProjectPointToNavigation.diagnostic_Z1000','/remote/object/call',
                                        {'objectPath':navigation,'functionName':'K2_ProjectPointToNavigation',
                                         'parameters':{'WorldContextObject':context['pawn'],'Point':candidate,
                                             'NavData':None,'FilterClass':None,'QueryExtent':diagnostic['diagnostic_query_extent']},
                                         'generateTransaction':False})
                                    diagnostic['wide_z_projection']={'state':'READ','response':wide}
                                    if wide.get('ReturnValue') is True:
                                        wide_point=vector(wide.get('ProjectedLocation'))
                                        diagnostic['wide_z_projection'].update(actual_nav_point=wide_point,
                                            delta_z_cm=wide_point['Z']-candidate['Z'])
                                except Exception as wide_error:
                                    diagnostic['wide_z_projection']={'state':'NOT_READ_OR_PARTIAL','error':str(wide_error)}
                                    diagnostic['state']='READ_ONLY_PARTIAL'
                                save_report()
                            raise RuntimeError('Bounded query candidate has no actual generated nav projection; no fallback')
                        point=vector(projected.get('ProjectedLocation'))
                        segment['actual_projected_goal']=point
                        if point==previous_projected or distance(current,point)<210:
                            raise RuntimeError('Bounded navigation projected a repeated or already-reached point; no guessed onward movement')
                        segment['state']='WALKING_COMPLETE_PROJECTED_PATH'
                        save_report()
                        walk_to(point)
                        arrived=position()
                        segment.update(state='PROJECTED_POINT_REACHED',actual_arrival=arrived,
                                       actual_displacement_cm=distance(current,arrived),
                                       distance_at_arrival_cm=distance(arrived,point))
                        if distance(current,arrived)<=20:
                            raise RuntimeError('Bounded navigation made no observed movement beyond the existing 20cm progress threshold')
                        previous_projected=point
                        save_report()
                else:
                    walk_to(goal)
                arrived=position()
                sample.update(state='FINAL_TARGET_REACHED',arrived_position=arrived,
                              distance_at_arrival_cm=distance(arrived,goal))
                report['guidance_state']='FINAL_TARGET_REACHED'
                save_report()
                return
            route_id=guidance.get('route_id')
            coordinates=guidance.get('route_world')
            if not isinstance(route_id,str) or route_id in ('','None') or not isinstance(coordinates,list) or len(coordinates)!=3:
                raise RuntimeError('Actual guidance route ID or coordinates missing')
            goal=vector(dict(zip('XYZ',coordinates)))
            sample.update(route_id=route_id,route_world=goal,distance_to_node_cm=distance(current,goal))
            if reached and route_id==reached['route_id'] and goal==reached['route_world']:
                report['guidance_state']='NODE_REACHED_WITHOUT_TRANSITION_STOPPED'
                raise RuntimeError('Actual guidance node was reached but its ID and coordinates did not change; no guessed onward movement')
            sample['state']='WALKING_ACTUAL_NODE'
            sample['arrival_cm']=50
            walk_to(goal,arrival_cm=50)
            arrived=position()
            sample.update(state='NODE_REACHED',arrived_position=arrived,distance_at_arrival_cm=distance(arrived,goal))
            reached=sample
            save_report()

    try:
        launch = ue.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap',extra_args=[
            '-game','-RCWebControlEnable','-HearthwardSaveTestPool='+pool,'-UserDir='+str(profile),'-AbsLog='+str(log),
            '-RemoteControlINI='+str(policy),'-ini:RemoteControl:[/Script/RemoteControlCommon.RemoteControlSettings]:RemoteControlHttpServerPort='+str(port),
            '-ini:RemoteControl:[/Script/RemoteControlCommon.RemoteControlSettings]:bAutoStartWebSocketServer=False',
            '-windowed','-ResX=1920','-ResY=1080','-ForceRes','-NoSplash','-dx12'])
        (out/'launch.json').write_text(json.dumps(launch,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        pid = launch.get('payload',{}).get('process_id')
        if not launch.get('ok') or not pid:
            raise RuntimeError('Public UEClient did not return successful owned launch')
        report.update(status='RUNNING',process_id=pid)
        report['stage'] = 'bootstrap_title'
        while True:
            try:
                http('rc.info','/remote/info');break
            except (error.URLError,TimeoutError,BadStatusLine):
                guard();time.sleep(.5)
        context = references('/Game/Hearthward/Bootstrap/L_Bootstrap.L_Bootstrap')
        if context['level'] != 'L_Bootstrap' or call(context['screen'],'GetPage') != 'title':
            raise RuntimeError('Normal Bootstrap title is not the actual entry')
        report['bootstrap'] = context.copy()
        loading = subsystem('HearthwardLoadingSubsystem',True)
        wait_loading(loading)
        if call(context['screen'],'ExecuteAction',{'Action':'new'}) is not True:
            raise RuntimeError('Real menu new-game action was refused')
        while True:
            try:
                current = references('/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds.L_HearthwardWilds')
                if current['level']=='L_HearthwardWilds':
                    context=current;break
            except (error.HTTPError,RuntimeError):
                pass
            guard();time.sleep(.5)
        report['natural'] = context.copy()
        loading = subsystem('HearthwardLoadingSubsystem',True)
        wait_loading(loading)
        camp, save = subsystem('HearthwardCampaignSubsystem'), subsystem('HearthwardSaveSubsystem')
        player_gameplay = ref(prop(context['pawn'],'Gameplay'))
        combat = ref(call(context['pawn'],'GetComponentByClass',{'ComponentClass':'/Script/Hearthward.HearthwardCombatComponent'}))
        timed = ref(call(context['pawn'],'GetComponentByClass',{'ComponentClass':'/Script/Hearthward.HearthwardTimedActionComponent'}))
        survival = ref(call(context['pawn'],'GetComponentByClass',{'ComponentClass':'/Script/Hearthward.HearthwardSurvivalComponent'}))
        while campaign_busy():
            time.sleep(.2)
        deadline=time.monotonic()+args.route_timeout
        report['stage']='actual_artifact'
        safe_input();state=campaign()
        if state.get('phase')!='prologue' or 'relic' in state.get('facts',[]):
            raise RuntimeError('Fresh normal prologue baseline does not match')
        walk_to(vector(state['positions']['prologue_relic']))
        safe_input()
        if call(camp,'Interact') is not True or 'relic' not in campaign().get('facts',[]):
            raise RuntimeError('Ordinary nearest artifact interaction did not earn relic fact')
        screenshot('earned_artifact')
        report['stage']='ordinary_follow_directive'
        if call(player_gameplay,'OrderCompanion',{'Order':'follow'}) is not True or 'prologue_order' not in campaign().get('facts',[]):
            raise RuntimeError('Normal direct follow instruction was refused')
        report['stage']='actual_escape_walk'
        walk_guided_escape()
        report['stage']='ordinary_escape_interaction'
        safe_input()
        if call(camp,'Interact') is not True:
            raise RuntimeError('Ordinary nearest exit interaction was refused')
        while True:
            state=campaign()
            busy=campaign_busy()
            if state.get('phase')=='occupied' and 'prologue_complete' in state.get('facts',[]) and not busy:
                break
            if not busy:
                report['interaction_prompt']=call(camp,'Prompt')
                raise RuntimeError('Exit did not begin or complete genuine travel')
            time.sleep(.25)
        report['earned_escape']={'phase':state['phase'],'facts':state['facts'],'position':position()}
        report['stage']='camp_loading_completion'
        report['loading_after_travel']=call(loading,'IsLoading')
        wait_loading(loading)
        safe_input();report['stage']='genuine_safe_save'
        before=call(save,'GetPoints')
        saved=call(context['screen'],'ExecuteAction',{'Action':'save'})
        report['save_status']=call(save,'GetStatus')
        after=call(save,'GetPoints')
        report['save_transaction']={'accepted':saved,'points_before':before,'points_after':after}
        if saved is not True:
            raise RuntimeError('Real safe SavePoint transaction refused; no raw checkpoint manufactured')
        report['earned_checkpoint']={'phase':state['phase'],'facts':state['facts'],'position':position(),'campaign_id':call(save,'GetCampaignId')}
        screenshot('earned_camp_checkpoint')
        report.update(status='PROLOGUE_EARNED_CHECKPOINT_API_VERIFIED',stage='complete')
    except Exception as exc:
        report.update(status='FAILED_FIRST_BLOCKER',first_error=str(exc))
        if context:
            try:
                deadline=max(deadline,time.monotonic()+10)
                screenshot('first_blocker')
            except Exception as capture_error:
                report['screenshot_error']=str(capture_error)
    finally:
        save_report()
        if pid:
            try:
                stopped=ue.runtime.stop_editor(process_id=pid)
                (out/'stop.json').write_text(json.dumps(stopped,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
                report['owned_editor_stop']=stopped
                if not stopped.get('ok'):
                    raise RuntimeError('Public UEClient refused owned process stop')
                stop_deadline=time.monotonic()+10
                while process_alive() and time.monotonic()<stop_deadline:
                    time.sleep(.2)
                report['owned_editor_exit_verified']=not process_alive()
                if not report['owned_editor_exit_verified']:
                    raise RuntimeError('Owned UE process did not exit within 10s')
            except Exception as exc:
                report['owned_editor_stop_error']=str(exc)
        save_report()
    print(json.dumps({'status':report['status'],'stage':report['stage'],'evidence':str(out/'results.json')},ensure_ascii=False))
    return 0 if report['status']=='PROLOGUE_EARNED_CHECKPOINT_API_VERIFIED' and report.get('owned_editor_exit_verified') is True else 1


if __name__=='__main__':
    raise SystemExit(main())
