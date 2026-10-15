"""One stable normal-new dialogue diagnostic, adapted from TASK-072's tested public RC host.
Root owns actual execution. No world fixture, model confirmation, normal OS input or six-scene acceptance.
CSV parser/statistics/log analysis are copied unchanged from TASK-102/run_prologue.py.
"""
import argparse
import csv
import math
import json
import os
import socket
import sys
import time
import uuid
from pathlib import Path
from urllib import error, request
import ctypes
from ctypes import wintypes
from datetime import datetime, timedelta
import io
import re
import subprocess
import shutil

OTHER_SCENES = [
    {'scene':'camp_day','status':'NOT_RUN','missing':'A real safe-camp checkpoint with actual facilities/population and observed daylight.'},
    {'scene':'camp_night','status':'NOT_RUN','missing':'A real safe-camp checkpoint and paid real Sleep/Wait facility receipt producing observed night; no clock injection.'},
    {'scene':'three_person_combat','status':'NOT_RUN','missing':'Actual player, brother and enemy combat maintained over the sample without invulnerability or fake damage.'},
    {'scene':'route_streaming','status':'NOT_RUN','missing':'An unlocked real route/checkpoint, actual character movement and WorldPartition cell-load evidence; no teleport within capture.'},
    {'scene':'camp_management','status':'NOT_RUN','missing':'A real safe camp with facilities/workers/active production, actual management UI and MenuPause false.'},
]


def process_running(pid):
    """Query only the owned PID through Win32; never read a process command line."""
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD,wintypes.BOOL,wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.GetExitCodeProcess.argtypes = [wintypes.HANDLE,ctypes.POINTER(wintypes.DWORD)]
    kernel.GetExitCodeProcess.restype = wintypes.BOOL
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    handle = kernel.OpenProcess(0x1000,False,int(pid))
    if not handle:
        code = ctypes.get_last_error()
        if code == 87:
            return False
        raise ctypes.WinError(code)
    try:
        exit_code = wintypes.DWORD()
        if not kernel.GetExitCodeProcess(handle,ctypes.byref(exit_code)):
            raise ctypes.WinError(ctypes.get_last_error())
        return exit_code.value == 259
    finally:
        kernel.CloseHandle(handle)


def parse_frame_timings(content):
    """Keep every numeric frame; accept only UE's known final header/metadata rows."""
    csv.field_size_limit(16*1024*1024)
    rows = list(csv.reader(io.StringIO(content)))
    if not rows or rows[0].count('FrameTime') != 1:
        raise ValueError('Expected one FrameTime column in the engine CSV header')
    header = rows[0]
    column = header.index('FrameTime')
    last_row = max(i for i, row in enumerate(rows) if row)
    timings = []
    metadata = None
    for number, row in enumerate(rows[1:], 1):
        if not row:
            continue
        if row[0] == 'EVENTS' and row[:len(header)] == header:
            continue
        if (number == last_row and len(row) >= 4
                and row[:3] == ['[HasHeaderRowAtEnd]', '1', '[EventTimestamps]']
                and row[3] in ['0', '1']):
            metadata = {}
            position = 0
            while position < len(row):
                key = row[position]
                if not re.fullmatch(r'\[[^\]]+\]', key) or position+1 >= len(row):
                    raise ValueError(f'Malformed engine CSV metadata at row {number+1}')
                # UE writes commandline last without CSV escaping its command-list commas.
                if key.lower() == '[commandline]':
                    metadata['Commandline'] = ','.join(row[position+1:])
                    break
                metadata[key[1:-1]] = row[position+1]
                position += 2
            continue
        if len(row) <= column:
            raise ValueError(f'Missing FrameTime at engine CSV row {number+1}')
        try:
            value = float(row[column])
        except ValueError as error:
            raise ValueError(f'Invalid FrameTime at engine CSV row {number+1}: {row[column]!r}') from error
        if not math.isfinite(value) or value < 0:
            raise ValueError(f'Invalid FrameTime at engine CSV row {number+1}: {row[column]!r}')
        timings.append(value)
    if metadata is None or not timings:
        raise ValueError('Missing complete engine CSV metadata or numeric frame timings')
    return timings, metadata

def frame_statistics(timings):
    ordered = sorted(timings)
    # Preserve TASK-072's approved sample-index and floor-count definitions.
    worst = ordered[-max(1, len(ordered)//100):]
    slowest_mean = sum(worst)/len(worst)
    if slowest_mean == 0:
        raise ValueError('Cannot calculate 1% Low from an all-zero slowest sample')
    return {'numeric_frames':len(timings), 'seconds':sum(timings)/1000,
            'p99_ms':ordered[min(len(ordered)-1, int((len(ordered)-1)*.99))],
            'one_percent_low_fps':1000/slowest_mean,
            'over_50ms':sum(t>50 for t in timings),
            'over_50ms_fraction':sum(t>50 for t in timings)/len(timings),
            'statistics_definition':'TASK-072: p99 index floor((n-1)*0.99); 1% Low slowest max(1,n//100) frames'}

def analyze_model_log(source):
    """Log-observed diagnostics only: an HTTP response is not a completed game UI."""
    stamp_pattern = re.compile(r'^\[(\d{4}\.\d{2}\.\d{2}-\d{2}\.\d{2}\.\d{2}:\d{3})\]')
    startups = []
    generations = []
    failures = []
    capture_started = capture_stop = boot_stamp = None
    boot_offset = None
    for line in source.splitlines():
        stamp = stamp_pattern.match(line)
        if not stamp:
            continue
        at = datetime.strptime(stamp[1], '%Y.%m.%d-%H.%M.%S:%f')
        if 'LogCsvProfiler:' in line:
            boot = re.search(r'Doing a boot time capture\. Start time offset ([0-9.]+)', line)
            if boot and capture_started is None:
                boot_stamp, boot_offset = at, float(boot[1])
            if 'Capture started. CSV ID:' in line and capture_started is None:
                capture_started = at
            if 'Capture Stop requested' in line and capture_started is not None and capture_stop is None:
                capture_stop = at
        started = re.search(r'Local AI runtime started pid=(\d+) backend=(\w+)', line)
        ready = re.search(r'Local AI runtime ready pid=(\d+)', line)
        generation = re.search(r'Local AI generation request #(\d+) tier=(\S+) input_tokens=(\d+)', line)
        if started:
            startups.append({'pid':int(started[1]), 'backend':started[2], 'started_log_at':at.isoformat(),
                             'ready_log_at':None, 'start_to_ready_seconds':None})
        if ready:
            runtime = next((item for item in reversed(startups) if item['pid'] == int(ready[1])
                            and item['ready_log_at'] is None), None)
            if runtime is not None:
                runtime['ready_log_at'] = at.isoformat()
                runtime['start_to_ready_seconds'] = (at-datetime.fromisoformat(runtime['started_log_at'])).total_seconds()
        if generation:
            generations.append({'observed_request':len(generations)+1, 'generation':int(generation[1]),
                                'tier':generation[2], 'input_tokens':int(generation[3]),
                                'started_log_at':at.isoformat(), 'http_terminal_log_at':None,
                                'http_terminal_kind':None, 'generation_to_http_terminal_seconds':None,
                                'submission_to_http_terminal_seconds':None})
        http_failed = re.search(r'Local AI HTTP failed: success=(\d+) code=(\d+) elapsed=([0-9.]+)s', line)
        response = re.search(r'Local AI timings: generation=(\d+) submission_to_response=([0-9.]+)s', line)
        if (http_failed or response) and generations and generations[-1]['http_terminal_log_at'] is None:
            request = generations[-1]
            if http_failed or int(response[1]) == request['generation']:
                request['http_terminal_log_at'] = at.isoformat()
                request['http_terminal_kind'] = 'HTTP_FAILED' if http_failed else 'HTTP_RESPONSE_TIMINGS'
                request['generation_to_http_terminal_seconds'] = (at-datetime.fromisoformat(request['started_log_at'])).total_seconds()
                request['submission_to_http_terminal_seconds'] = float(http_failed[3] if http_failed else response[2])
                if http_failed:
                    request['http_success'] = int(http_failed[1])
                    request['http_code'] = int(http_failed[2])
        failure = re.search(r'Local AI failure \[([^\]]+)\]: (.*)', line)
        if failure:
            failures.append({'log_at':at.isoformat(), 'reason_code':failure[1], 'detail':failure[2]})
    capture_start = (boot_stamp-timedelta(seconds=boot_offset) if boot_stamp is not None else capture_started)
    window_known = capture_start is not None and capture_stop is not None and capture_stop >= capture_start
    for request in generations:
        begin = datetime.fromisoformat(request['started_log_at'])
        end = datetime.fromisoformat(request['http_terminal_log_at']) if request['http_terminal_log_at'] else None
        request['generation_start_in_capture'] = capture_start <= begin <= capture_stop if window_known else None
        request['capture_contains_whole_generation_interval'] = (
            capture_start <= begin <= end <= capture_stop if window_known and end is not None else None)
        request['whole_capture_under_generation'] = (
            begin <= capture_start and end >= capture_stop if window_known and end is not None else None)
        request['overlap_seconds'] = (max(0.0, (min(end, capture_stop)-max(begin, capture_start)).total_seconds())
                                      if window_known and end is not None else None)
        request['http_terminal_not_observed'] = end is None
    return {'timing_basis':'Same engine log timestamps; no UI completion or TTFT inferred',
            'capture_start_estimate_log_at':capture_start.isoformat() if capture_start else None,
            'capture_started_marker_log_at':capture_started.isoformat() if capture_started else None,
            'capture_stop_marker_log_at':capture_stop.isoformat() if capture_stop else None,
            'boot_start_offset_seconds':boot_offset,
            'capture_boundary_precision':'Boot start estimated from offset rounded to 0.01s; stop marker may include end-frame bookkeeping',
            'capture_window_known':window_known, 'runtime_startups':startups,
            'observed_generation_count':len(generations), 'generations':generations, 'failures':failures,
            'capture_contains_observed_generation':any(item['generation_start_in_capture'] is True for item in generations)
                if window_known else None,
            'capture_contains_whole_observed_request':(
                True if any(item['capture_contains_whole_generation_interval'] is True for item in generations)
                else None if any(item['http_terminal_not_observed'] for item in generations)
                else False) if window_known else None,
            'game_ui_completion':'NOT_MEASURED; requires normal UI evidence',
            'cold_start_benchmark':'NOT_EVALUATED; recorded start-to-ready only',
            'warm_p95':'NOT_RUN', 'ttft':'NOT_RUN; non-streaming response'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, default=Path(__file__).resolve().parents[3]/'Hearthward.uproject')
    parser.add_argument('--ue-root', type=Path, default=Path('G:/UnrealEngine/UE_5.8'))
    parser.add_argument('--backend', choices=['cpu', 'vulkan'], required=True)
    parser.add_argument('--run', required=True, help='New isolated TASK-102 output directory name.')
    parser.add_argument('--bundle', type=Path)
    parser.add_argument('--timeout', type=int, default=600)
    parser.add_argument('--checkpoint', type=Path, help='Read-only source pool copied into this isolated profile; use the normal Continue action.')
    parser.add_argument('--output-root', type=Path, help='Optional evidence root for later task consumers.')
    parser.add_argument('--camp-wait-minutes', type=int, choices=[60,240,480], help='Before sampling, use the existing campfire UI action with its normal costs and safety checks; never set the clock directly.')
    parser.add_argument('--confirm-proposal', action='store_true', help='After the frame sample, confirm the actual collect proposal through its UI action and verify real delivery.')
    parser.add_argument('--scene', choices=['camp','camp-management','route-streaming','combined-combat','rain-stealth'], default='camp', help='Actual camp, authored-route follow, field guards or natural rain; no transform writes.')
    parser.add_argument('--warm-requests', type=int, choices=[0,20,60], default=0, help='Collect sequential warm requests: 20 is diagnostic, 60 meets the component-window sample minimum. Retain semantic refusals; no viewport paint credit.')
    args = parser.parse_args()
    if not 0 < args.timeout <= 600:
        parser.error('--timeout must be 1..600 seconds')
    if Path(args.run).name != args.run or args.run in ('.', '..'):
        parser.error('--run must be one new directory name')
    project = args.project.resolve()
    if not project.is_file():
        parser.error('--project must name an existing .uproject')
    bundle = (args.bundle or project.parent/'Runtime/LocalAI').resolve()
    for required in [bundle/'models/Qwen3.5-4B-Q4_K_M.gguf', bundle/'bin'/args.backend/'llama-server.exe']:
        if not required.is_file():
            parser.error('Existing locked runtime file is missing: '+str(required))
    configured = os.environ.get('HEARTHWARD_FACTORY_ROOT')
    candidates = [Path(configured)] if configured else []
    candidates.extend(project.parent.parents)
    factory = next((p for p in candidates if (p/'engine_adapters/ue5/__init__.py').is_file()), None)
    if factory is None:
        parser.error('Set HEARTHWARD_FACTORY_ROOT to the prepared GameFactory checkout')
    sys.path.insert(0, str(factory.resolve()))
    from engine_adapters.ue5 import UEClient
    pool = str(uuid.uuid4())
    out = (args.output_root or project.parent/'.agent-local/qa/TASK-102')/args.run
    out = out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    user_dir = out/'profile'
    if args.checkpoint:
        checkpoint = args.checkpoint.resolve(strict=True)
        destination = user_dir/'Saved/SaveGames/HearthwardPrototype/Compatible-v8/pool.hws'
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(checkpoint, destination)
    log = out/'runtime.log'
    with socket.socket() as port_probe:
        port_probe.bind(('127.0.0.1', 0))
        port = port_probe.getsockname()[1]
    ue = UEClient(project_path=project, ue_root=args.ue_root, host='127.0.0.1', port=port)
    opener = request.build_opener(request.ProxyHandler({}))
    overall_deadline = time.monotonic()+args.timeout
    deadline = overall_deadline-15
    pid = None
    csv_started = False
    cleanup = False
    last_resource = 0
    capture_observed_at = None
    report = {'run':args.run, 'pool':pool, 'output':str(out), 'backend_requested':args.backend,
              'status':'NOT_RUN', 'stage':'launch', 'timeout_seconds':args.timeout,
              'scene':'normal_new_initial_scene_dialogue_fixed_view',
              'evidence_level':'Uncooked standalone -game; public API injected UI/model input; stable one-scene diagnostic',
              'acceptance':'NOT_EVALUATED', 'six_scene_joint_matrix':'NOT_RUN',
              'shipping_credit':False, 'os_input_credit':False, 'owner_acceptance':'NOT_RUN',
              'model_parameter_changes':bool(os.environ.get('LLAMA_ARG_UBATCH')),
              'requested_ubatch_environment':os.environ.get('LLAMA_ARG_UBATCH'),
              'actor_position_changes':False, 'progress_fixture_changes':False,
              'model_request_count':0, 'references':{}, 'observations':{}, 'frame_exclusions':[],
              'warm_p95':'NOT_RUN', 'ttft':'NOT_RUN; non-streaming', 'ui_first_paint_latency':'NOT_RUN',
              'other_scene_conditions':OTHER_SCENES}
    head = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=project.parent, capture_output=True, text=True, encoding='utf-8')
    report['source_head'] = head.stdout.strip() if head.returncode == 0 else None
    report['build_source_binding'] = 'NOT_VERIFIED_BY_RUNNER; bind actual build/candidate evidence separately'
    if args.checkpoint:
        report.update(scene='continued_checkpoint_dialogue_fixed_view', checkpoint=str(checkpoint))
    old_model_log = os.environ.get('LLAMA_ARG_LOG_FILE')

    def ensure_running():
        if cleanup or pid is None:
            return
        if log.is_file():
            source = log.read_text(encoding='utf-8-sig')
            if any(marker in source for marker in ['=== Critical error:', 'Fatal error!', 'Unhandled Exception:']):
                report['runtime_failure_kind'] = 'FATAL_LOG'
                raise RuntimeError('Owned UE log reports fatal/critical runtime failure')
        if not process_running(pid):
            report['runtime_failure_kind'] = 'OWNED_UE_PROCESS_EXITED'
            raise RuntimeError('Owned UE process exited before completion')

    def sample_resources():
        nonlocal last_resource
        if time.monotonic()-last_resource < 10 or pid is None:
            return
        last_resource = time.monotonic()
        source = log.read_text(encoding='utf-8-sig') if log.is_file() else ''
        model_pids = list(dict.fromkeys(int(value) for value in re.findall(r'Local AI runtime started pid=(\d+)', source)))
        process_filter = ' OR '.join('ProcessId='+str(value) for value in [pid]+model_pids)
        command = '@{Processes=@(Get-CimInstance Win32_Process -Filter "'+process_filter+'" | Select-Object ProcessId,WorkingSetSize,PageFileUsage);Memory=(Get-CimInstance Win32_OperatingSystem | Select-Object FreePhysicalMemory,TotalVisibleMemorySize,TotalVirtualMemorySize,FreeVirtualMemory)} | ConvertTo-Json -Depth 3 -Compress'
        entry = {'at_monotonic':time.monotonic(), 'time':time.time(), 'ue_pid':pid, 'model_pids':model_pids,
                 'scope':'CIM process RAM/pagefile + OS free/virtual RAM + whole GPU telemetry; no per-process VRAM attribution'}
        for label, command_line in [('memory',['powershell','-NoProfile','-Command',command]),
                                    ('gpu',['nvidia-smi','--query-gpu=timestamp,temperature.gpu,power.draw,memory.used,utilization.gpu','--format=csv,noheader'])]:
            try:
                result = subprocess.run(command_line, capture_output=True, text=True, encoding='utf-8', timeout=5)
                entry[label] = {'exit_code':result.returncode, 'output':result.stdout.strip(), 'error':result.stderr.strip()}
            except (OSError, subprocess.TimeoutExpired) as exc:
                entry[label] = {'error':repr(exc)}
        with (out/'resources.jsonl').open('a', encoding='utf-8') as stream:
            stream.write(json.dumps(entry, ensure_ascii=False)+'\n')

    def write_report():
        (out/'results.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


    def http(name, route, body=None):
        ensure_running()
        sample_resources()
        if time.monotonic() >= deadline:
            raise TimeoutError('Joint diagnostic deadline reached at '+name)
        started = time.monotonic()
        entry = {'name': name, 'stage': report['stage'], 'method': 'GET' if body is None else 'PUT',
                 'route': route, 'request': body, 'started_monotonic': started}
        try:
            req = request.Request('http://127.0.0.1:'+str(port)+route,
                                  data=None if body is None else json.dumps(body).encode('utf-8'),
                                  headers={'Content-Type': 'application/json'}, method=entry['method'])
            request_timeout = 45 if name in ('natural.current_level', 'natural.loading') else 10
            with opener.open(req, timeout=min(request_timeout, max(.1, deadline-started))) as response:
                entry['http_status'] = response.status
                raw = response.read().decode('utf-8-sig')
            value = json.loads(raw)
            entry['response'] = value
            if entry['http_status'] != 200 or not isinstance(value, dict):
                raise RuntimeError('Unexpected RC status/JSON object at '+name)
            return value
        except error.HTTPError as exc:
            entry.update(http_status=exc.code, response_text=exc.read().decode('utf-8-sig'))
            raise
        except Exception as exc:
            entry['error'] = repr(exc)
            raise
        finally:
            entry['elapsed_seconds'] = time.monotonic()-started
            with (out/'http-events.jsonl').open('a', encoding='utf-8') as journal:
                journal.write(json.dumps(entry, ensure_ascii=False)+'\n')


    def call(name, obj, function, parameters=None):
        return http(name, '/remote/object/call', {'objectPath': obj, 'functionName': function,
                                                'parameters': parameters or {}, 'generateTransaction': False})


    def ref(value, name):
        if not isinstance(value, str) or not value.startswith('/'):
            raise RuntimeError('Missing actual UObject reference at '+name+': '+repr(value))
        return value


    def player_references(context, prefix):
        pawn = ref(call(prefix+'.pawn', gameplay, 'GetPlayerPawn',
                        {'WorldContextObject': context, 'PlayerIndex': 0}).get('ReturnValue'), prefix+'.pawn')
        level = ref(call(prefix+'.level', pawn, 'GetLevel').get('ReturnValue'), prefix+'.level')
        world = ref(call(prefix+'.world_outer', system, 'GetOuterObject', {'Object': level}).get('ReturnValue'), prefix+'.world_outer')
        if call(prefix+'.world_path', system, 'GetPathName', {'Object': world}).get('ReturnValue') != world:
            raise RuntimeError('Actual world path mismatch at '+prefix)
        current = call(prefix+'.level_name', gameplay, 'GetCurrentLevelName',
                       {'WorldContextObject': pawn, 'bRemovePrefixString': True}).get('ReturnValue')
        pc = ref(call(prefix+'.controller', gameplay, 'GetPlayerController',
                      {'WorldContextObject': pawn, 'PlayerIndex': 0}).get('ReturnValue'), prefix+'.controller')
        hud = ref(call(prefix+'.hud', pc, 'GetHUD').get('ReturnValue'), prefix+'.hud')
        screen = ref(http(prefix+'.public_screen_read', '/remote/object/property',
                          {'objectPath': hud, 'propertyName': 'Screen', 'access': 'READ_ACCESS'}).get('Screen'), prefix+'.screen')
        return {'pawn': pawn, 'level': level, 'world': world, 'current_level': current,
                'controller': pc, 'hud': hud, 'screen': screen}
    gameplay = '/Script/Engine.Default__GameplayStatics'
    system = '/Script/Engine.Default__KismetSystemLibrary'
    subsystems = '/Script/Engine.Default__SubsystemBlueprintLibrary'
    allowed_calls = {
        '/Script/Engine.KismetSystemLibrary': ['IsValid','GetOuterObject','GetPathName','ExecuteConsoleCommand','GetProjectSavedDirectory','GetConsoleVariableStringValue'],
        '/Script/Engine.GameplayStatics': ['GetPlayerPawn','GetCurrentLevelName','GetPlayerController','GetGameInstance','GetAllActorsOfClass','IsGamePaused'],
        '/Script/Hearthward.HearthwardCharacter': ['GetLevel','GetComponentByClass','K2_GetActorLocation','GetActorForwardVector','Crouch'],
        '/Script/AIModule.AIBlueprintHelperLibrary': ['SimpleMoveToLocation'],
        '/Script/NavigationSystem.NavigationSystemV1': ['K2_ProjectPointToNavigation','FindPathToLocationSynchronously'],
        '/Script/NavigationSystem.NavigationPath': ['IsValid','IsPartial'],
        '/Script/Hearthward.HearthwardGameplayComponent': ['OrderCompanion','InCombat','GetCompanionCombatTarget','GetCompanionCombatReason'],
        '/Script/Hearthward.HearthwardCombatComponent': ['Attack','Busy','SetGuard'],
        '/Script/Hearthward.HearthwardCampaignSubsystem': ['Describe'],
        '/Script/Hearthward.HearthwardCampaignActor': ['K2_GetActorLocation'],
        '/Script/Hearthward.HearthwardWeatherActor': ['GetComponentByClass'],
        '/Script/Engine.InstancedStaticMeshComponent': ['IsVisible'],
        '/Script/Hearthward.HearthwardInteractionComponent': ['GetNearestTarget','InteractNearest','GetCompletionFeedback'],
        '/Script/Hearthward.HearthwardNatureSubsystem': ['Busy'],
        '/Script/Hearthward.HearthwardNatureInteraction': ['GetOwner'],
        '/Script/Hearthward.HearthwardNatureActor': ['K2_GetActorLocation'],
        '/Script/Hearthward.HearthwardStorageSubsystem': ['GetItemCount'],
        '/Script/Hearthward.HearthwardBuildingComponent': ['SelectBuilding','ConfirmPlacement','IsBuilding'],
        '/Script/Engine.PlayerController': ['GetHUD','StopMovement','GetControlRotation','SetControlRotation'],
        '/Script/Engine.SubsystemBlueprintLibrary': ['GetGameInstanceSubsystem','GetWorldSubsystem'],
        '/Script/Hearthward.HearthwardScreenWidget': ['GetPage','ExecuteAction','GetCurrentDialogueReply','CaptureUI'],
        '/Script/Hearthward.HearthwardLoadingSubsystem': ['IsLoading'],
        '/Script/Hearthward.HearthwardSaveSubsystem': ['IsNaturalWorldEnabled','GetCampaignId'],
        '/Script/Hearthward.HearthwardWorldClockSubsystem': ['GetSnapshot'],
        '/Script/Hearthward.HearthwardCampSubsystem': ['Describe'],
        '/Script/Hearthward.HearthwardCompanionFixture': ['CanCommunicate','GetPhase','GetDelivered','GetAcquired','GetCarried','GetExecutionAction','K2_GetActorLocation','GetComponentByClass'],
        '/Script/Hearthward.HearthwardInventoryComponent': ['GetItemCount'],
        '/Script/Hearthward.HearthwardLocalAISubsystem': ['IsBusy','IsModelReady','GetGenerationCalls','GetServerProcessId','SubmitPlayerText','GetStatus','GetLastInput','GetLastStructuredResult','GetLastLatencySeconds','GetInputTokens','GetOutputTokens','GetLastFilteredContext','GetContextTier','GetReasonCode','GetLastAppliedIntent','HasCandidate','GetCandidate','GetCandidateId'],
    }
    policy_ini = out/'RemoteControl.ini'
    policy_lines = ['[/Script/RemoteControlCommon.RemoteControlSettings]',
                    'bAllowAnyRemoteFunctionCall=False','bAutoStartWebSocketServer=False',
                    'bAllowConsoleCommandRemoteExecution=True','RemoteControlHttpServerPort='+str(port)]
    policy_lines += ['CustomAllowedRemoteFunctionCalls=(ClassPath="'+cls+'",FunctionName=("'+function+'"),bAllowChildClasses=False)'
                     for cls, functions in allowed_calls.items() for function in functions]
    policy_ini.write_text('\n'.join(policy_lines)+'\n', encoding='utf-8')
    report['remote_function_policy'] = {'allow_any':False,'child_classes':False,'exact_rules':allowed_calls,'file':str(policy_ini),'scope':'owned session ini only'}
    try:
        os.environ['LLAMA_ARG_LOG_FILE'] = str(out/'local-ai.log')
        launch_args = ['-game','-RCWebControlEnable','-HearthwardSaveTestPool='+pool,
                      '-ini:RemoteControl:[/Script/RemoteControlCommon.RemoteControlSettings]:RemoteControlHttpServerPort='+str(port),
                      '-ini:RemoteControl:[/Script/RemoteControlCommon.RemoteControlSettings]:bAutoStartWebSocketServer=False',
                      '-RemoteControlINI='+str(policy_ini),'-UserDir='+str(user_dir),'-AbsLog='+str(log),
                      '-windowed','-ResX=1920','-ResY=1080','-NoSplash','-unattended','-dx12','-ForceRes','-NoVSync','-csvCompression=0','-csvGpuStats',
                      '-HearthwardAIBackend='+args.backend,'-HearthwardAIGpuLayers=16','-HearthwardAIBundlePath='+str(bundle)]
        if args.checkpoint:
            launch_args.remove('-HearthwardSaveTestPool='+pool)
        launch = ue.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap', extra_args=launch_args)
        pid = launch.get('payload',{}).get('process_id')
        (out/'launch.json').write_text(json.dumps(launch,ensure_ascii=False,indent=2)+'\n', encoding='utf-8')
        report.update(process_id=pid,status='RUNNING')
        write_report()
        if not launch.get('ok') or not pid:
            raise RuntimeError('UEClient launch failed or returned no owned PID')
        report['stage'] = 'wait_rc_info'
        while True:
            try:
                info = http('rc.info', '/remote/info')
                break
            except error.HTTPError:
                raise
            except (error.URLError, TimeoutError):
                if time.monotonic() >= deadline:
                    raise
                time.sleep(.5)
        routes = {(row.get('Path'), str(row.get('Verb')).lower()) for row in info.get('HttpRoutes', [])}
        if not any(path == '/remote/object/call' for path, verb in routes):
            raise RuntimeError('RC object/call route was not advertised')
        report['stage'] = 'bootstrap_seed_attempt'
        seed = '/Game/Hearthward/Bootstrap/L_Bootstrap.L_Bootstrap'
        report['bootstrap_seed_attempt'] = {'path': seed, 'source': 'requested map asset object path; unverified until actual pawn/world returned',
                                            'validated': False, 'engine_path_attempt': None}
        if call('bootstrap.seed_valid', system, 'IsValid', {'Object': seed}).get('ReturnValue') is not True:
            raise RuntimeError('Requested Bootstrap World seed is not an existing live reference; no guessed fallback')
        boot = player_references(seed, 'bootstrap')
        if boot['current_level'] != 'L_Bootstrap' or boot['world'] != seed:
            raise RuntimeError('Actual pawn/world does not match the requested Bootstrap World')
        report['bootstrap_seed_attempt']['validated'] = True
        report['references']['bootstrap'] = boot
        instance = ref(call('bootstrap.game_instance', gameplay, 'GetGameInstance',
                            {'WorldContextObject': boot['pawn']}).get('ReturnValue'), 'bootstrap.game_instance')
        report['references']['game_instance'] = instance
        if call('bootstrap.page', boot['screen'], 'GetPage').get('ReturnValue').casefold() != 'title':
            raise RuntimeError('Normal Bootstrap title was not ready')
        report['stage'] = 'wait_bootstrap_loading'
        boot_loading = ref(call('bootstrap.loading_subsystem', subsystems, 'GetGameInstanceSubsystem',
                                {'ContextObject': instance, 'Class': '/Script/Hearthward.HearthwardLoadingSubsystem'}).get('ReturnValue'), 'bootstrap.loading_subsystem')
        while True:
            boot_loading_now = call('bootstrap.loading', boot_loading, 'IsLoading').get('ReturnValue')
            if not isinstance(boot_loading_now, bool):
                raise RuntimeError('Bootstrap loading getter did not return a Boolean')
            if not boot_loading_now:
                break
            time.sleep(.5)
        entry_action = 'continue' if args.checkpoint else 'new'
        report['stage'] = 'normal_'+entry_action
        if call('bootstrap.normal_'+entry_action, boot['screen'], 'ExecuteAction', {'Action': entry_action}).get('ReturnValue') is not True:
            raise RuntimeError('Normal '+entry_action+' action was rejected')
        report['stage'] = 'wait_natural_map'
        while True:
            current = call('natural.current_level', gameplay, 'GetCurrentLevelName',
                           {'WorldContextObject': instance, 'bRemovePrefixString': True}).get('ReturnValue')
            if current == 'L_HearthwardWilds':
                break
            if current not in ('L_Bootstrap', '', 'None'):
                raise RuntimeError('Normal new entered unexpected level '+repr(current))
            time.sleep(.5)
        natural = player_references(instance, 'natural')
        if natural['current_level'] != 'L_HearthwardWilds' or natural['world'] == boot['world']:
            raise RuntimeError('Natural actor/world identity was not renewed')
        report['references']['natural'] = natural
        loading = ref(call('natural.loading_subsystem', subsystems, 'GetGameInstanceSubsystem',
                           {'ContextObject': instance, 'Class': '/Script/Hearthward.HearthwardLoadingSubsystem'}).get('ReturnValue'), 'natural.loading_subsystem')
        report['references']['loading'] = loading
        report['stage'] = 'wait_normal_loading'
        while True:
            loading_now = call('natural.loading', loading, 'IsLoading').get('ReturnValue')
            if not isinstance(loading_now, bool):
                raise RuntimeError('Loading getter did not return a Boolean')
            if not loading_now:
                break
            time.sleep(.5)
        if call('natural.page', natural['screen'], 'GetPage').get('ReturnValue').casefold() != 'hud':
            raise RuntimeError('Normal new did not finish at HUD')
        report['stage'] = 'public_actual_references'
        for label, cls in [('players', 'HearthwardCharacter'), ('companions', 'HearthwardCompanionFixture')]:
            rows = call('natural.'+label, gameplay, 'GetAllActorsOfClass',
                        {'WorldContextObject': natural['pawn'], 'ActorClass': '/Script/Hearthward.'+cls}).get('OutActors')
            if not isinstance(rows, list) or len(rows) != 1:
                raise RuntimeError('Expected one actual '+cls+', observed '+repr(rows))
            report['references'][label] = [ref(rows[0], label)]
        if report['references']['players'][0] != natural['pawn']:
            raise RuntimeError('Public player list does not identify the current actual pawn')
        ai = ref(call('natural.local_ai', subsystems, 'GetWorldSubsystem',
                      {'ContextObject': natural['pawn'], 'Class': '/Script/Hearthward.HearthwardLocalAISubsystem'}).get('ReturnValue'), 'natural.local_ai')
        save = ref(call('natural.save', subsystems, 'GetWorldSubsystem',
                        {'ContextObject': natural['pawn'], 'Class': '/Script/Hearthward.HearthwardSaveSubsystem'}).get('ReturnValue'), 'natural.save')
        report['references'].update(local_ai=ai, save=save)
        clock = ref(call('natural.clock', subsystems, 'GetWorldSubsystem',
                         {'ContextObject': natural['pawn'], 'Class': '/Script/Hearthward.HearthwardWorldClockSubsystem'}).get('ReturnValue'), 'natural.clock')
        report['observations']['clock'] = call('natural.clock_snapshot', clock, 'GetSnapshot').get('ReturnValue')
        if args.camp_wait_minutes:
            economy = ref(call('prepare.camp', subsystems, 'GetWorldSubsystem',
                {'ContextObject': natural['pawn'], 'Class': '/Script/Hearthward.HearthwardCampSubsystem'}).get('ReturnValue'), 'prepare.camp')
            before = json.loads(call('prepare.camp_before', economy, 'Describe').get('ReturnValue'))
            fires = [f for f in before['facilities'] if f['kind'] == 'campfire' and f['camp'] == 'camp' and not f['paused']]
            report['camp_wait'] = {'minutes_requested': args.camp_wait_minutes, 'before': before,
                                   'clock_before': report['observations']['clock'], 'scope':'Normal campfire UI action; no clock or state injection.'}
            if not fires:
                builder = ref(call('prepare.builder', natural['pawn'], 'GetComponentByClass',
                    {'ComponentClass':'/Script/Hearthward.HearthwardBuildingComponent'}).get('ReturnValue'), 'prepare.builder')
                report['camp_wait']['construction'] = {'scope':'Actual placement preview, collision checks, reserved materials and timed completion; no fixture grants.'}
                bag = ref(call('prepare.bag', natural['pawn'], 'GetComponentByClass',
                    {'ComponentClass':'/Script/Hearthward.HearthwardInventoryComponent'}).get('ReturnValue'), 'prepare.bag')
                shared = ref(call('prepare.shared', subsystems, 'GetWorldSubsystem',
                    {'ContextObject':natural['pawn'], 'Class':'/Script/Hearthward.HearthwardStorageSubsystem'}).get('ReturnValue'), 'prepare.shared')
                interaction = ref(call('prepare.interaction', natural['pawn'], 'GetComponentByClass',
                    {'ComponentClass':'/Script/Hearthward.HearthwardInteractionComponent'}).get('ReturnValue'), 'prepare.interaction')
                nature = ref(call('prepare.nature', subsystems, 'GetWorldSubsystem',
                    {'ContextObject':natural['pawn'], 'Class':'/Script/Hearthward.HearthwardNatureSubsystem'}).get('ReturnValue'), 'prepare.nature')
                def position():
                    return call('prepare.position', natural['pawn'], 'K2_GetActorLocation').get('ReturnValue')
                def move_to(goal):
                    call('prepare.walk', '/Script/AIModule.Default__AIBlueprintHelperLibrary', 'SimpleMoveToLocation',
                        {'Controller':natural['controller'], 'Goal':goal})
                    walking_deadline = min(deadline, time.monotonic()+60)
                    while True:
                        here = position()
                        if math.hypot(here['X']-goal['X'], here['Y']-goal['Y']) <= 140:
                            call('prepare.stop_walk', natural['controller'], 'StopMovement')
                            time.sleep(.5)
                            return
                        if time.monotonic() >= walking_deadline:
                            raise RuntimeError('Actual camp resource navigation did not reach its goal')
                        time.sleep(.5)
                start_position = position()
                report['camp_wait']['harvest'] = []
                for item in ('wood','stone'):
                    def count():
                        return sum(call('prepare.count_'+item, container, 'GetItemCount', {'ItemId':item}).get('ReturnValue') for container in (bag,shared))
                    if count() >= 4:
                        continue
                    sources = [s for s in before['sources'] if s['camp']=='camp' and s['item']==item and s['remaining']>=4
                               and ('fallen_branches' in s['id'] if item=='wood' else 'loose_stones' in s['id'])]
                    if not sources:
                        raise RuntimeError('No actual camp resource can supply '+item)
                    here = position()
                    resource = min(sources, key=lambda s:math.hypot(s['position']['x']-here['X'],s['position']['y']-here['Y']))
                    move_to({k.upper():v for k,v in resource['position'].items()})
                    for attempt in range(4):
                        if count() >= 4:
                            break
                        target = ref(call('prepare.nearest', interaction, 'GetNearestTarget').get('ReturnValue'), 'prepare.nearest')
                        owner = ref(call('prepare.resource_owner', target, 'GetOwner').get('ReturnValue'), 'prepare.resource_owner')
                        observed = call('prepare.resource_position', owner, 'K2_GetActorLocation').get('ReturnValue')
                        if math.hypot(observed['X']-resource['position']['x'],observed['Y']-resource['position']['y']) > 60:
                            raise RuntimeError('Nearest actual nature interaction is not at the selected camp resource')
                        previous_count = count()
                        if call('prepare.harvest', interaction, 'InteractNearest').get('ReturnValue') is not True:
                            raise RuntimeError('Normal timed harvest rejected')
                        while call('prepare.harvest_busy', nature, 'Busy').get('ReturnValue'):
                            time.sleep(.5)
                        gained_count = count()
                        report['camp_wait']['harvest'].append({'source':resource['id'],'before':previous_count,'after':gained_count,
                            'feedback':call('prepare.harvest_feedback', interaction, 'GetCompletionFeedback').get('ReturnValue')})
                        if gained_count <= previous_count:
                            raise RuntimeError('Timed harvest produced no usable material')
                    if count() < 4:
                        raise RuntimeError('Actual harvest still has insufficient '+item)
                if report['camp_wait']['harvest']:
                    move_to(start_position)
                    if call('prepare.save_materials', natural['screen'], 'ExecuteAction', {'Action':'save'}).get('ReturnValue') is not True:
                        raise RuntimeError('Harvested materials could not be saved normally')
                    harvested = out/'harvested-camp.hws'
                    shutil.copy2(user_dir/'Saved/SaveGames/HearthwardPrototype/Compatible-v8/pool.hws', harvested)
                    report['camp_wait']['harvested_checkpoint'] = str(harvested)
                if call('prepare.select_campfire', builder, 'SelectBuilding', {'Id':'campfire'}).get('ReturnValue') is not True:
                    raise RuntimeError('Normal campfire selection rejected')
                time.sleep(1)
                placement = http('prepare.placement', '/remote/object/property',
                    {'objectPath':builder, 'propertyName':'ValidPlacement', 'access':'READ_ACCESS'}).get('ValidPlacement')
                if placement is not True:
                    view = call('prepare.view', natural['controller'], 'GetControlRotation').get('ReturnValue')
                    report['camp_wait']['construction']['view_before'] = view
                    for offset in (90,180,270):
                        turned = dict(view, Yaw=view['Yaw']+offset)
                        call('prepare.turn_view', natural['controller'], 'SetControlRotation', {'NewRotation':turned})
                        time.sleep(.5)
                        placement = http('prepare.placement', '/remote/object/property',
                            {'objectPath':builder, 'propertyName':'ValidPlacement', 'access':'READ_ACCESS'}).get('ValidPlacement')
                        if placement is True:
                            report['camp_wait']['construction']['view_after'] = turned
                            break
                report['camp_wait']['construction']['valid_placement'] = placement
                if placement is not True:
                    report['camp_wait']['construction']['feedback'] = http('prepare.build_feedback', '/remote/object/property',
                        {'objectPath':builder, 'propertyName':'Feedback', 'access':'READ_ACCESS'})
                    raise RuntimeError('Normal campfire placement is unavailable; see actual building feedback')
                if call('prepare.confirm_campfire', builder, 'ConfirmPlacement').get('ReturnValue') is not True:
                    raise RuntimeError('Normal campfire construction rejected')
                while call('prepare.building', builder, 'IsBuilding').get('ReturnValue'):
                    time.sleep(.5)
                built = json.loads(call('prepare.camp_built', economy, 'Describe').get('ReturnValue'))
                fires = [f for f in built['facilities'] if f['kind'] == 'campfire' and f['camp'] == 'camp' and not f['paused']]
                report['camp_wait']['construction']['facilities_after'] = built['facilities']
                if not fires:
                    raise RuntimeError('Construction ended without a completed campfire')
            for action in ('page:camp', 'camp.facility:' + fires[0]['id'], 'camp.wait:' + str(args.camp_wait_minutes)):
                if call('prepare.action', natural['screen'], 'ExecuteAction', {'Action':action}).get('ReturnValue') is not True:
                    raise RuntimeError('Normal campfire preparation rejected: ' + action)
            report['camp_wait']['after'] = json.loads(call('prepare.camp_after', economy, 'Describe').get('ReturnValue'))
            report['camp_wait']['clock_after'] = call('prepare.clock_after', clock, 'GetSnapshot').get('ReturnValue')
            elapsed = report['camp_wait']['after']['calendar'] - before['calendar']
            if elapsed < args.camp_wait_minutes:
                raise RuntimeError('Campfire wait did not complete the requested interval; no daylight sample credit')
            if call('prepare.save', natural['screen'], 'ExecuteAction', {'Action':'save'}).get('ReturnValue') is not True:
                raise RuntimeError('Prepared camp could not create a normal save point')
            prepared = out/'prepared-camp.hws'
            shutil.copy2(user_dir/'Saved/SaveGames/HearthwardPrototype/Compatible-v8/pool.hws', prepared)
            report['camp_wait']['prepared_checkpoint'] = str(prepared)
        if call('natural.enabled', save, 'IsNaturalWorldEnabled').get('ReturnValue') is not True:
            raise RuntimeError('Formal natural-world session is not enabled')
        report['observations']['campaign_id'] = call('natural.campaign_id', save, 'GetCampaignId').get('ReturnValue')
        for field, function in [('busy','IsBusy'),('ready','IsModelReady'),('generation_calls','GetGenerationCalls'),('server_process_id','GetServerProcessId')]:
            report['observations'][field] = call('ai.'+field, ai, function).get('ReturnValue')
        if report['observations']['busy'] is not False or report['observations']['ready'] is not False or report['observations']['generation_calls'] != 0 or report['observations']['server_process_id'] != 0:
            raise RuntimeError('Fresh normal-new session contains unexpected prior AI activity')
        report['stage'] = 'model_csv_preconditions'
        probe = {'scene': 'normal_new_initial_scene_dialogue_fixed_view', 'backend_requested': args.backend,
                 'bundle': str(bundle), 'gpu_layers_requested': 16, 'public_submit_calls': 0,
                 'text': '采集1份木材送入营地仓库', 'csv_minimum_seconds': 60,
                 'warm_p95': 'NOT_RUN', 'ttft': 'NOT_RUN', 'ui_first_paint_latency': 'NOT_RUN',
                 'process_memory_vram_available_ram': 'NOT_RUN', 'task102_acceptance': 'NOT_EVALUATED'}
        report['model_csv'] = probe
        companion = report['references']['companions'][0]
        if call('probe.can_communicate', companion, 'CanCommunicate', {'Speaker': natural['pawn']}).get('ReturnValue') is not True:
            raise RuntimeError('Normal-new companion is not actually communicable; no fixture relocation')
        source = ref(http('probe.public_source_read', '/remote/object/property',
                          {'objectPath': companion, 'propertyName': 'Source', 'access': 'READ_ACCESS'}).get('Source'), 'probe.source')
        camp = ref(http('probe.public_camp_read', '/remote/object/property',
                        {'objectPath': companion, 'propertyName': 'Camp', 'access': 'READ_ACCESS'}).get('Camp'), 'probe.camp')
        source_wood = call('probe.source_before', source, 'GetItemCount', {'ItemId': 'wood'}).get('ReturnValue')
        if type(source_wood) is not int or source_wood < 1:
            raise RuntimeError('Actual normal-new Source has no wood for the explicit request')
        probe.update(source=source, camp=camp, source_wood_before=source_wood)
        if call('probe.dialogue', natural['screen'], 'ExecuteAction', {'Action': 'page:dialogue'}).get('ReturnValue') is not True:
            raise RuntimeError('Normal dialogue action rejected')
        if call('probe.page', natural['screen'], 'GetPage').get('ReturnValue').casefold() != 'dialogue':
            raise RuntimeError('Normal dialogue page is not open')
        if call('probe.paused_before', gameplay, 'IsGamePaused', {'WorldContextObject': natural['pawn']}).get('ReturnValue') is not False:
            raise RuntimeError('Normal dialogue paused the world; no meaningful joint frame sample')
        sample_page = 'dialogue'
        if args.scene == 'camp-management':
            report['scene'] = 'camp_management'
            economy = ref(call('management.camp', subsystems, 'GetWorldSubsystem',
                {'ContextObject': natural['pawn'], 'Class': '/Script/Hearthward.HearthwardCampSubsystem'}).get('ReturnValue'), 'management.camp')
            before = json.loads(call('management.before', economy, 'Describe').get('ReturnValue'))
            probe['management'] = {'scope': 'Existing MenuPause=false option; real camp facilities and assigned production, public API submission while panel remains open.', 'before': before}
            if not before['facilities']:
                raise RuntimeError('Management baseline requires actual completed facilities')
            for action in ('page:camp', 'camp.tab:workers', 'camp.region:camp_forage', 'camp.assign'):
                if call('management.prepare', natural['screen'], 'ExecuteAction', {'Action': action}).get('ReturnValue') is not True:
                    raise RuntimeError('Actual management preparation rejected: '+action)
            if call('management.paused', gameplay, 'IsGamePaused', {'WorldContextObject': natural['pawn']}).get('ReturnValue'):
                if call('management.menu_pause_option', natural['screen'], 'ExecuteAction', {'Action':'menuPause'}).get('ReturnValue') is not True:
                    raise RuntimeError('Existing menu pause option rejected')
            assigned = json.loads(call('management.assigned', economy, 'Describe').get('ReturnValue'))
            region = next(r for r in assigned['regions'] if r['id'] == 'camp_forage')
            if not region['workers']:
                raise RuntimeError('No actual ordinary worker assigned')
            if not region['enabled']:
                if call('management.enable', natural['screen'], 'ExecuteAction', {'Action':'camp.toggle'}).get('ReturnValue') is not True:
                    raise RuntimeError('Actual production could not be enabled')
            probe['management']['capture_before'] = json.loads(call('management.capture_before', economy, 'Describe').get('ReturnValue'))
            sample_page = 'camp'
        route = None
        encounter = None
        def read_property(name, obj, field):
            return http(name,'/remote/object/property',{'objectPath':obj,'propertyName':field,'access':'READ_ACCESS'}).get(field)

        if args.scene in ('route-streaming','combined-combat','rain-stealth'):
            report['scene'] = 'authored_route_follow_streaming'
            route_points = json.loads((project.parent/'Resources/Data/gameplay.json').read_text(encoding='utf-8'))['campaign']['route']
            route = {'goal_xy': [v*100 for v in route_points[1]], 'samples': [], 'loaded_levels': [],
                     'scope': 'Real authored camp-to-fork navigation and brother follow; no cooperative-device or normal keyboard credit.',
                     'goal_reached': False, 'next_goal': None, 'next_goal_at': 0, 'next_levels_at': 0}
            probe['route'] = route
            player_gameplay = ref(call('route.gameplay', natural['pawn'], 'GetComponentByClass',
                {'ComponentClass':'/Script/Hearthward.HearthwardGameplayComponent'}).get('ReturnValue'), 'route.gameplay')
            if call('route.follow', player_gameplay, 'OrderCompanion', {'Order':'follow'}).get('ReturnValue') is not True:
                raise RuntimeError('Normal companion follow order rejected')
            if call('route.hud', natural['screen'], 'ExecuteAction', {'Action':'page:hud'}).get('ReturnValue') is not True:
                raise RuntimeError('Normal HUD action rejected')
            sample_page = 'hud'
            if args.scene != 'route-streaming':
                report['scene'] = args.scene.replace('-','_')
                encounter = {'scope':'Actual field guards, damage and natural weather; no invulnerability or timeline injection.', 'enemies':[], 'samples':[]}
                probe['encounter'] = encounter
                actors = call('encounter.actors',gameplay,'GetAllActorsOfClass',
                    {'WorldContextObject':natural['pawn'],'ActorClass':'/Script/Hearthward.HearthwardCampaignActor'}).get('OutActors',[])
                for actor in actors:
                    identity = read_property('encounter.identity',actor,'Identity')
                    if str(identity).startswith('field_slice_'):
                        target = ref(read_property('encounter.target',actor,'Target'),'encounter.target')
                        if read_property('encounter.health',target,'Health') > 0:
                            encounter['enemies'].append({'id':identity,'actor':actor,'target':target})
                if not encounter['enemies']:
                    raise RuntimeError('No actual living field guards loaded from the earned route checkpoint')
                encounter['enemies'].sort(key=lambda row:row['id'])
                campaign = ref(call('encounter.campaign',subsystems,'GetWorldSubsystem',
                    {'ContextObject':natural['pawn'],'Class':'/Script/Hearthward.HearthwardCampaignSubsystem'})['ReturnValue'],'encounter.campaign')
                campaign_state = json.loads(call('encounter.campaign_state',campaign,'Describe')['ReturnValue'])
                home = next(e['home'] for e in campaign_state['enemies'] if e['id']==encounter['enemies'][0]['id'])
                first = {k.upper():v for k,v in home.items()}
                encounter['station_home'] = first
                encounter['next_attack_at'] = 0
                route['goal_xy'] = [first['X']-4000,first['Y']]
                route['scope'] = 'Normal approach to field guards before capture; route-follow counts are recorded separately.'
                combat = ref(call('encounter.player_combat',natural['pawn'],'GetComponentByClass',
                    {'ComponentClass':'/Script/Hearthward.HearthwardCombatComponent'})['ReturnValue'],'encounter.player_combat')
                brother_survival = ref(call('encounter.brother_survival',companion,'GetComponentByClass',
                    {'ComponentClass':'/Script/Hearthward.HearthwardSurvivalComponent'})['ReturnValue'],'encounter.brother_survival')
                weather = call('encounter.weather',gameplay,'GetAllActorsOfClass',
                    {'WorldContextObject':natural['pawn'],'ActorClass':'/Script/Hearthward.HearthwardWeatherActor'}).get('OutActors',[])
                if len(weather)!=1:
                    raise RuntimeError('Expected one actual world weather actor')
                drops = ref(call('encounter.rain_mesh',weather[0],'GetComponentByClass',
                    {'ComponentClass':'/Script/Engine.InstancedStaticMeshComponent'})['ReturnValue'],'encounter.rain_mesh')

        def route_poll(move):
            pos = call('route.position', natural['pawn'], 'K2_GetActorLocation').get('ReturnValue')
            brother_pos = call('route.brother_position', companion, 'K2_GetActorLocation').get('ReturnValue')
            now = time.monotonic()
            distance = math.hypot(route['goal_xy'][0]-pos['X'], route['goal_xy'][1]-pos['Y'])
            gap = math.dist([pos[k] for k in ('X','Y','Z')], [brother_pos[k] for k in ('X','Y','Z')])
            route['samples'].append({'at':now,'position':pos,'brother':brother_pos,'remaining_cm':distance,'brother_distance_cm':gap})
            if now >= route['next_levels_at']:
                offset = log.stat().st_size
                call('route.loaded_levels',system,'ExecuteConsoleCommand',
                    {'WorldContextObject':natural['pawn'],'SpecificPlayer':natural['controller'],'Command':'DumpLevelCollections'})
                dump_deadline = time.monotonic()+3
                while True:
                    with log.open('rb') as stream:
                        stream.seek(offset)
                        dump = stream.read().decode('utf-8-sig')
                    levels = sorted(set(re.findall(r'LogWorld:\s+Level (\S+)',dump)))
                    counts = [int(n) for n in re.findall(r'LogWorld: \d+: (\d+) levels\.',dump)]
                    if counts and len(levels) >= sum(counts):
                        break
                    if time.monotonic() >= dump_deadline:
                        levels = []
                        break
                    time.sleep(.1)
                if not levels:
                    raise RuntimeError('Public DumpLevelCollections produced no level identities; no streaming credit')
                route['loaded_levels'].append({'at':now,'levels':levels,'log_offset':offset,'source':'UE public DumpLevelCollections'})
                route['next_levels_at'] = now+10
            if not move:
                return
            if distance <= 180:
                route['goal_reached'] = True
                call('route.arrival',natural['controller'],'StopMovement')
                return
            if gap > (2800 if args.scene == 'rain-stealth' else 1600):
                call('route.wait_brother',natural['controller'],'StopMovement')
                route['next_goal'] = None
                return
            goal = route['next_goal']
            if goal and math.hypot(goal['X']-pos['X'],goal['Y']-pos['Y']) > 150:
                if now-route['next_goal_at'] > 20:
                    raise RuntimeError('Actual navigation did not reach its projected segment within20s; no teleport fallback')
                return
            scale = min(2000/distance,1)
            desired = {'X':pos['X']+(route['goal_xy'][0]-pos['X'])*scale,
                       'Y':pos['Y']+(route['goal_xy'][1]-pos['Y'])*scale,'Z':pos['Z']}
            projected = call('route.project','/Script/NavigationSystem.Default__NavigationSystemV1','K2_ProjectPointToNavigation',
                {'WorldContextObject':natural['pawn'],'Point':desired,'QueryExtent':{'X':400,'Y':400,'Z':20000}})
            if projected.get('ReturnValue') is not True:
                raise RuntimeError('Authored route segment has no generated navigation projection')
            goal = projected['ProjectedLocation']
            if encounter:
                # A projected island may be disconnected. Choose only an actual complete path.
                heading = math.atan2(route['goal_xy'][1]-pos['Y'],route['goal_xy'][0]-pos['X'])
                connected = None
                for angle in (0,30,-30,60,-60):
                    radians = heading+math.radians(angle)
                    candidate = {'X':pos['X']+min(distance,1000)*math.cos(radians),
                                 'Y':pos['Y']+min(distance,1000)*math.sin(radians),'Z':pos['Z']}
                    projection = call('route.reachable_project','/Script/NavigationSystem.Default__NavigationSystemV1','K2_ProjectPointToNavigation',
                        {'WorldContextObject':natural['pawn'],'Point':candidate,'QueryExtent':{'X':200,'Y':200,'Z':20000}})
                    if projection.get('ReturnValue') is not True:
                        continue
                    candidate = projection['ProjectedLocation']
                    path = call('route.path','/Script/NavigationSystem.Default__NavigationSystemV1','FindPathToLocationSynchronously',
                        {'WorldContextObject':natural['pawn'],'PathStart':pos,'PathEnd':candidate,'PathfindingContext':natural['pawn']}).get('ReturnValue')
                    if isinstance(path,str) and path.startswith('/'):
                        valid = call('route.path_valid',path,'IsValid').get('ReturnValue')
                        partial = call('route.path_partial',path,'IsPartial').get('ReturnValue')
                        if valid is True and partial is False:
                            connected = candidate
                            break
                if connected is None:
                    raise RuntimeError('No complete forward navigation path among five bounded approach directions')
                goal = connected
            call('route.walk','/Script/AIModule.Default__AIBlueprintHelperLibrary','SimpleMoveToLocation',
                 {'Controller':natural['controller'],'Goal':goal})
            call('route.face',natural['controller'],'SetControlRotation',{'NewRotation':{'Pitch':0,'Yaw':math.degrees(math.atan2(goal['Y']-pos['Y'],goal['X']-pos['X'])),'Roll':0}})
            route['next_goal'],route['next_goal_at'] = goal,now

        if route:
            route_poll(False)
        if encounter:
            report['stage'] = 'walk_to_actual_field_guards'
            while not route['goal_reached']:
                route_poll(True)
                time.sleep(1)
            call('encounter.approach_stop',natural['controller'],'StopMovement')
            encounter['approach_completed_at'] = time.monotonic()
            encounter['approach_save_accepted'] = call('encounter.save_approach',natural['screen'],'ExecuteAction',{'Action':'save'}).get('ReturnValue')
            if encounter['approach_save_accepted'] is True:
                shutil.copy2(user_dir/'Saved/SaveGames/HearthwardPrototype/Compatible-v8/pool.hws',out/'earned-approach.hws')
                encounter['earned_approach_checkpoint'] = str(out/'earned-approach.hws')
            if args.scene == 'rain-stealth':
                call('encounter.crouch',natural['pawn'],'Crouch',{'bClientSimulation':False})
                if call('encounter.wait_order',player_gameplay,'OrderCompanion',{'Order':'wait'}).get('ReturnValue') is not True:
                    raise RuntimeError('Normal brother wait order rejected before stealth approach')
                report['stage'] = 'wait_natural_rain'
                while call('encounter.rain_clear',drops,'IsVisible').get('ReturnValue') is True:
                    time.sleep(1)
                while call('encounter.rain_wait',drops,'IsVisible').get('ReturnValue') is not True:
                    time.sleep(1)
                encounter['rain_observed_before_capture'] = True
                route['goal_xy'] = [first['X']-2350,first['Y']]
                route['goal_reached'],route['next_goal'] = False,None
            else:
                route['goal_reached'] = True

        def encounter_poll():
            pos = route['samples'][-1]['position']
            sample = {'at':time.monotonic(),'player_health':read_property('encounter.player_health',player_gameplay,'Health'),
                      'brother_health':read_property('encounter.brother_health',brother_survival,'BrotherHealth'),
                      'player_combat':call('encounter.in_combat',player_gameplay,'InCombat').get('ReturnValue'),
                      'brother_target':call('encounter.brother_target',player_gameplay,'GetCompanionCombatTarget').get('ReturnValue'),
                      'brother_reason':call('encounter.brother_reason',player_gameplay,'GetCompanionCombatReason').get('ReturnValue'),
                      'player_forward':call('encounter.forward',natural['pawn'],'GetActorForwardVector').get('ReturnValue'),
                      'rain_visible':call('encounter.rain_visible',drops,'IsVisible').get('ReturnValue'),'enemies':[]}
            for enemy in encounter['enemies']:
                target_pos = call('encounter.enemy_position',enemy['actor'],'K2_GetActorLocation')['ReturnValue']
                memory = read_property('encounter.enemy_memory',enemy['target'],'Memory')
                sample['enemies'].append({'id':enemy['id'],'position':target_pos,
                    'health':read_property('encounter.enemy_health',enemy['target'],'Health'),
                    'memory':memory,'distance_cm':math.dist([pos[k] for k in ('X','Y','Z')],[target_pos[k] for k in ('X','Y','Z')])})
            encounter['samples'].append(sample)
            if args.scene == 'rain-stealth' and not encounter.get('retreat_started_at'):
                if not sample['rain_visible'] or any(any(value>.05 for value in e['memory']['Detection']) for e in sample['enemies']):
                    encounter['retreat_started_at'] = sample['at']
                    encounter['retreat_reason'] = 'Natural rain ended or actual enemy detection warning for either character rose above0.05'
                    route['goal_xy'] = [first['X']-4000,first['Y']]
                    route['goal_reached'],route['next_goal'] = False,None
            if args.scene == 'combined-combat' and sample['player_health']>0:
                living = [e for e in sample['enemies'] if e['health']>0]
                if living:
                    enemy = min(living,key=lambda e:e['distance_cm'])
                    destination = enemy['position']
                    call('encounter.face',natural['controller'],'SetControlRotation',{'NewRotation':{'Pitch':0,'Yaw':math.degrees(math.atan2(destination['Y']-pos['Y'],destination['X']-pos['X'])),'Roll':0}})
                    if enemy['distance_cm']>220:
                        horizontal = math.hypot(destination['X']-pos['X'],destination['Y']-pos['Y'])
                        approach = dict(destination)
                        if horizontal>180:
                            for key in ('X','Y'):
                                approach[key] -= (destination[key]-pos[key])*180/horizontal
                        call('encounter.approach','/Script/AIModule.Default__AIBlueprintHelperLibrary','SimpleMoveToLocation',
                             {'Controller':natural['controller'],'Goal':approach})
                    else:
                        call('encounter.attack_stop',natural['controller'],'StopMovement')
                        if sample['at'] >= encounter['next_attack_at']:
                            sample['attack_accepted'] = call('encounter.attack',combat,'Attack',{'Heavy':False}).get('ReturnValue')
                            if sample['attack_accepted'] is True:
                                encounter['next_attack_at'] = sample['at']+4
                        else:
                            sample['guard_accepted'] = call('encounter.guard',combat,'SetGuard',{'Value':True}).get('ReturnValue')

        settings = [('sg.'+name+'Quality', 3) for name in ['ViewDistance', 'AntiAliasing', 'Shadow', 'GlobalIllumination', 'Reflection', 'PostProcess', 'Texture', 'Effects', 'Foliage', 'Shading', 'Landscape']]
        settings += [('r.ScreenPercentage', 100), ('r.DynamicRes.OperationMode', 0), ('t.MaxFPS', 0), ('r.VSync', 0), ('r.AntiAliasingMethod', 2), ('r.Shadow.Virtual.Enable', 1), ('r.Nanite', 1)]
        probe['render_cvars'] = {}
        for variable, expected in settings:
            call('probe.set.'+variable, system, 'ExecuteConsoleCommand',
                 {'WorldContextObject': natural['pawn'], 'SpecificPlayer': natural['controller'], 'Command': variable+' '+str(expected)})
            actual = call('probe.read.'+variable, system, 'GetConsoleVariableStringValue', {'VariableName': variable}).get('ReturnValue')
            probe['render_cvars'][variable] = actual
            if not isinstance(actual, str) or float(actual) != expected:
                raise RuntimeError('Approved render setting did not read back: '+variable+'='+repr(actual))
        report['stage'] = 'model_csv_settle'
        probe['excluded_before_capture_seconds'] = 10
        settle_end = time.monotonic()+10
        while time.monotonic() < settle_end:
            if call('probe.settle.loading', loading, 'IsLoading').get('ReturnValue') is not False:
                raise RuntimeError('Normal loading resumed before capture; sample not started')
            if call('probe.settle.page', natural['screen'], 'GetPage').get('ReturnValue').casefold() != sample_page:
                raise RuntimeError('Page changed during the pre-capture stable window')
            if call('probe.settle.paused', gameplay, 'IsGamePaused', {'WorldContextObject':natural['pawn']}).get('ReturnValue') is not False:
                raise RuntimeError('World paused during the pre-capture stable window')
            if call('probe.settle.communicate', companion, 'CanCommunicate', {'Speaker':natural['pawn']}).get('ReturnValue') is not True:
                raise RuntimeError('Communication failed during the pre-capture stable window')
            time.sleep(1)
        saved = call('probe.actual_saved_directory', system, 'GetProjectSavedDirectory').get('ReturnValue')
        if not isinstance(saved, str) or not Path(saved).is_absolute():
            raise RuntimeError('Public Saved directory is not an absolute path')
        csv_path = Path(saved)/'Profiling/CSV'/('Task102-'+pool+'.csv')
        if not csv_path.resolve().is_relative_to(out.resolve()):
            raise RuntimeError('UserDir did not isolate the actual CSV output under this session')
        probe['csv'] = str(csv_path)
        report['stage'] = 'model_csv_start'
        for command in ['CsvProfile STARTFILE=Task102-'+pool, 'CsvProfile START']:
            call('probe.csv_command', system, 'ExecuteConsoleCommand',
                 {'WorldContextObject': natural['pawn'], 'SpecificPlayer': natural['controller'], 'Command': command})
        csv_started = True
        file_start_deadline = min(deadline, time.monotonic()+10)
        while not csv_path.is_file():
            ensure_running()
            if time.monotonic() >= file_start_deadline:
                raise TimeoutError('CSV START returned without creating the unique capture file')
            time.sleep(.1)
        capture_observed_at = time.monotonic()
        probe['capture_started_observed_monotonic'] = capture_observed_at
        if args.scene == 'combined-combat':
            encounter['assist_order_accepted'] = call('encounter.assist',player_gameplay,'OrderCompanion',{'Order':'attack'}).get('ReturnValue')
            if encounter['assist_order_accepted'] is not True:
                raise RuntimeError('Actual brother assistance order rejected')
        report['stage'] = 'model_csv_single_submit'
        submitted_at = time.monotonic()
        probe['public_submit_calls'] = 1
        submitted = call('probe.single_submit', ai, 'SubmitPlayerText',
                         {'Speaker': natural['pawn'], 'Companion': companion, 'Text': probe['text']}).get('ReturnValue')
        probe['submit_returned_after_seconds'] = time.monotonic()-submitted_at
        if submitted is not True:
            raise RuntimeError('Actual public SubmitPlayerText rejected; no retries or replacement request')
        report['stage'] = 'model_csv_joint_sample'
        ready_observed_at = None
        terminal_observed_at = None
        busy_observed = False
        while True:
            if route:
                route_poll(args.scene != 'combined-combat')
            if encounter:
                encounter_poll()
            state = {field: call('probe.poll.'+field, ai, function).get('ReturnValue')
                     for field, function in [('busy', 'IsBusy'), ('ready', 'IsModelReady'), ('generation_calls', 'GetGenerationCalls'), ('status', 'GetStatus')]}
            observed_at = time.monotonic()
            if type(state['busy']) is not bool or type(state['ready']) is not bool or type(state['generation_calls']) is not int:
                raise RuntimeError('Public AI progress getter returned an unexpected type')
            if state['generation_calls'] > 1:
                raise RuntimeError('More than one actual generation observed for the single request')
            busy_observed = busy_observed or state['busy']
            if state['ready'] and ready_observed_at is None:
                ready_observed_at = observed_at
            if not state['busy'] and terminal_observed_at is None:
                terminal_observed_at = observed_at
            state.update(submission_elapsed_seconds=observed_at-submitted_at, capture_elapsed_seconds=observed_at-capture_observed_at)
            with (out/'model-progress.jsonl').open('a', encoding='utf-8') as journal:
                journal.write(json.dumps(state, ensure_ascii=False)+'\n')
            report['model_request_count'] = state['generation_calls']
            if call('probe.poll.loading', loading, 'IsLoading').get('ReturnValue') is not False:
                raise RuntimeError('Loading occurred inside the joint sample; no silent frame exclusion')
            if call('probe.poll.page', natural['screen'], 'GetPage').get('ReturnValue').casefold() != sample_page:
                raise RuntimeError('Scene page changed during sample')
            if call('probe.poll.paused', gameplay, 'IsGamePaused', {'WorldContextObject': natural['pawn']}).get('ReturnValue') is not False:
                raise RuntimeError('World pause state did not match the actual requested scene')
            if call('probe.poll.communicate', companion, 'CanCommunicate', {'Speaker':natural['pawn']}).get('ReturnValue') is not True:
                raise RuntimeError('Communication became invalid inside the joint sample')
            if terminal_observed_at is not None and observed_at-capture_observed_at >= 61 and (not route or route['goal_reached']):
                break
            time.sleep(1)
        probe.update(capture_started_observed_monotonic=capture_observed_at, submission_monotonic=submitted_at, terminal_observed_monotonic=terminal_observed_at,
                     capture_stop_requested_monotonic=time.monotonic(), busy_observed=busy_observed,
                     cold_ready_observed_seconds=None if ready_observed_at is None else ready_observed_at-submitted_at,
                     full_ue_terminal_observed_seconds=terminal_observed_at-submitted_at,
                     poll_interval_seconds=1, model_request_count=state['generation_calls'])
        report['stage'] = 'model_csv_stop'
        call('probe.csv_stop', system, 'ExecuteConsoleCommand',
             {'WorldContextObject': natural['pawn'], 'SpecificPlayer': natural['controller'], 'Command': 'CsvProfile STOP'})
        csv_started = False
        if route:
            call('route.stop',natural['controller'],'StopMovement')
            route_poll(False)
            original = set(route['loaded_levels'][0]['levels'])
            route['new_loaded_levels'] = sorted({level for row in route['loaded_levels'] for level in row['levels']}-original)
            route['streaming_transition_observed'] = bool(route['new_loaded_levels'])
            if call('route.save',natural['screen'],'ExecuteAction',{'Action':'save'}).get('ReturnValue') is True:
                pool_file = user_dir/'Saved/SaveGames/HearthwardPrototype/Compatible-v8/pool.hws'
                if pool_file.is_file():
                    shutil.copy2(pool_file,out/'earned-route.hws')
                    route['earned_checkpoint'] = str(out/'earned-route.hws')
        file_end_deadline = min(deadline, time.monotonic()+30)
        while True:
            ensure_running()
            try:
                content = csv_path.read_text(encoding='utf-8-sig')
            except PermissionError:
                content = ''
            if '[hasheaderrowatend]' in content.lower():
                break
            if time.monotonic() >= file_end_deadline:
                raise TimeoutError('CSV STOP has not completed final metadata/header; no partial-file frame credit')
            time.sleep(.5)
        if args.scene == 'camp-management':
            after = json.loads(call('management.after', economy, 'Describe').get('ReturnValue'))
            probe['management']['capture_after'] = after
            initial = next(r for r in probe['management']['capture_before']['regions'] if r['id'] == 'camp_forage')
            final = next(r for r in after['regions'] if r['id'] == 'camp_forage')
            progressed = final['completed'] > initial['completed'] or final['batch']['work'] > initial['batch']['work']
            probe['management']['actual_production_progressed'] = progressed
            if not progressed:
                raise RuntimeError('No actual production progress during management capture')
            call('management.return_dialogue', natural['screen'], 'ExecuteAction', {'Action':'page:dialogue'})
        report['stage'] = 'model_csv_terminal_results'
        terminal = {field: call('probe.terminal.'+field, ai, function).get('ReturnValue')
                    for field, function in [('last_input', 'GetLastInput'), ('raw', 'GetLastStructuredResult'), ('submission_to_response_seconds', 'GetLastLatencySeconds'), ('input_tokens', 'GetInputTokens'), ('output_tokens', 'GetOutputTokens'), ('reason', 'GetReasonCode'), ('applied_intent', 'GetLastAppliedIntent'), ('has_candidate', 'HasCandidate'), ('candidate', 'GetCandidate'), ('context_tier', 'GetContextTier'), ('server_process_id', 'GetServerProcessId')]}
        probe['terminal'] = terminal
        candidate = terminal['candidate'] if isinstance(terminal['candidate'], dict) else {}
        probe['expected_collect_candidate'] = (terminal['has_candidate'] is True and terminal['reason'] == '' and terminal['applied_intent'] == 'proposal'
                                                and str(candidate.get('Intent','')).casefold() == 'collect' and str(candidate.get('Item','')).casefold() == 'wood'
                                               and candidate.get('Quantity') == 1 and candidate.get('QuantityMode') == 'additional_acquired' and candidate.get('SourceRef') == 'S1')
        probe['source_wood_after'] = call('probe.source_after', source, 'GetItemCount', {'ItemId': 'wood'}).get('ReturnValue')
        probe['unconfirmed_source_unchanged'] = probe['source_wood_after'] == source_wood
        probe['render_cvars_after'] = {}
        for variable, expected in settings:
            actual = call('probe.read_after.'+variable, system, 'GetConsoleVariableStringValue', {'VariableName':variable}).get('ReturnValue')
            probe['render_cvars_after'][variable] = actual
            if not isinstance(actual, str) or float(actual) != expected:
                raise RuntimeError('Approved render setting changed during capture: '+variable+'='+repr(actual))
        timings, metadata = parse_frame_timings(content)
        frame_report = {**frame_statistics(timings), 'dropped_frames':0,'frame_exclusions':[],
                        'distribution':'Every numeric frame in this complete stable-capture CSV; no fixed boot trimming'}
        frame_report['diagnostic_frame_target_pass'] = (frame_report['p99_ms'] <= 16.67
            and frame_report['one_percent_low_fps'] >= 60 and frame_report['over_50ms_fraction'] <= .001)
        render_metadata = {key.lower():value for key,value in metadata.items() if key.lower() != 'commandline'}
        frame_report['csv_metadata'] = render_metadata
        actual_res = [render_metadata.get('systemresolution.resx'),render_metadata.get('systemresolution.resy')]
        frame_report['actual_system_resolution'] = actual_res
        frame_report['actual_resolution_matches'] = actual_res == ['1920','1080']
        frame_report['actual_rhi_matches'] = (render_metadata.get('rhiname') == 'D3D12'
            and render_metadata.get('rhifeaturelevel') == 'SM6' and render_metadata.get('vsyncenabled') == '0')
        probe['frame_report'] = frame_report
        (out/'frame-report.json').write_text(json.dumps(frame_report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        report['model_log_diagnostic'] = analyze_model_log(log.read_text(encoding='utf-8-sig'))
        probe['same_log_generation_interval_contained'] = report['model_log_diagnostic']['capture_contains_whole_observed_request']
        probe['model_terminal_observed'] = terminal_observed_at is not None and terminal['last_input'] == probe['text']
        probe['model_reply_received'] = probe['model_terminal_observed'] and bool(terminal['raw']) and not terminal['reason']
        probe['dialogue_reply_text'] = call('probe.visible_reply', natural['screen'], 'GetCurrentDialogueReply').get('ReturnValue')
        (Path(saved)/'Task020').mkdir(parents=True, exist_ok=True)
        capture_name = 'round2-' + args.run + '-terminal'
        probe['dialogue_widget_capture_ok'] = call('probe.capture_reply', natural['screen'], 'CaptureUI',
            {'Name':capture_name, 'Width':1920, 'Height':1080, 'PreserveFocus':True}).get('ReturnValue')
        if probe['dialogue_widget_capture_ok']:
            shutil.copy2(Path(saved)/'Task020'/(capture_name+'.png'), out/'dialogue-terminal.png')
        probe['dialogue_widget_capture_scope'] = 'Actual current '+sample_page+' widget rendered after the complete frame capture; not viewport first-paint latency or execution confirmation.'
        if args.confirm_proposal and probe['expected_collect_candidate']:
            shared = ref(call('execution.storage', subsystems, 'GetWorldSubsystem',
                {'ContextObject':natural['pawn'], 'Class':'/Script/Hearthward.HearthwardStorageSubsystem'}).get('ReturnValue'), 'execution.storage')
            execution = {'scope':'Normal UI confirmation and actual companion delivery after the frame sample; no state injection.'}
            probe['execution'] = execution
            execution['camp_wood_before'] = call('execution.camp_before', shared, 'GetItemCount', {'ItemId':'wood'}).get('ReturnValue')
            candidate_id = call('execution.candidate_id', ai, 'GetCandidateId').get('ReturnValue')
            if isinstance(candidate_id, dict) and set(candidate_id) == {'A','B','C','D'}:
                candidate_id = ''.join(format(candidate_id[key] & 0xffffffff, '08X') for key in ('A','B','C','D'))
            if not isinstance(candidate_id, str) or not re.fullmatch(r'[0-9A-Fa-f]{32}',candidate_id):
                raise RuntimeError('Candidate ID was not a valid reflected GUID')
            execution['accepted'] = call('execution.confirm', natural['screen'], 'ExecuteAction', {'Action':'agentConfirm:'+candidate_id}).get('ReturnValue')
            if execution['accepted'] is not True:
                raise RuntimeError('Actual proposal confirmation was rejected')
            execution_deadline = min(deadline, time.monotonic()+90)
            execution['observed_phases'] = []
            while True:
                phase = call('execution.phase', companion, 'GetPhase').get('ReturnValue')
                if not execution['observed_phases'] or execution['observed_phases'][-1] != phase:
                    execution['observed_phases'].append(phase)
                if phase in ('Completed','Cancelled','ReturningBlocked','HoldingSafely') or time.monotonic() >= execution_deadline:
                    break
                time.sleep(.5)
            execution['phase'] = phase
            execution['delivered'] = call('execution.delivered', companion, 'GetDelivered').get('ReturnValue')
            execution['source_wood_after'] = call('execution.source_after', source, 'GetItemCount', {'ItemId':'wood'}).get('ReturnValue')
            execution['camp_wood_after'] = call('execution.camp_after', shared, 'GetItemCount', {'ItemId':'wood'}).get('ReturnValue')
            execution['action'] = call('execution.action', companion, 'GetExecutionAction').get('ReturnValue')
            execution['passed'] = (phase == 'Completed' and execution['delivered'] == 1
                and execution['camp_wood_after'] == execution['camp_wood_before']+1
                and execution['source_wood_after'] == source_wood-1)
            if not execution['passed']:
                raise RuntimeError('Confirmed collection did not complete with the required source/storage conservation')
        probe['single_semantic_expectation'] = 'PASS' if probe['expected_collect_candidate'] else 'FAIL'
        report['configuration_scope'] = 'All render CVars read back before 10-second pre-capture stable window; actual resolution from completed CSV metadata'
        report['performance'] = 'ONE_SCENE_DIAGNOSTIC_ONLY'
        if frame_report['seconds'] < 60:
            raise RuntimeError('Complete CSV contains less than 60 real frame seconds')
        if not frame_report['actual_resolution_matches']:
            raise RuntimeError('CSV system resolution did not verify the approved actual 1920x1080')
        if not frame_report['actual_rhi_matches']:
            raise RuntimeError('Complete CSV did not verify actual D3D12/SM6/VSync off')
        if state['generation_calls'] != 1 or ready_observed_at is None or not busy_observed:
            raise RuntimeError('Joint sample did not observe exactly one real generation and busy/ready state')
        if probe['same_log_generation_interval_contained'] is not True:
            raise RuntimeError('Same engine log did not verify the whole generation-to-HTTP interval inside capture')
        if not probe['unconfirmed_source_unchanged']:
            raise RuntimeError('Source changed before any confirmation')
        if args.warm_requests:
            report['stage'] = 'warm_requests'
            report['cold_model_log_diagnostic'] = report['model_log_diagnostic']
            overall_deadline = time.monotonic()+130*args.warm_requests+15
            deadline = overall_deadline-15
            warm = {'scope':'Same process, ready model; production submission, validated terminal state and dialogue getter. No first-paint or execution frame credit.',
                    'requested':args.warm_requests,'samples':[], 'ui_first_paint':'NOT_RUN',
                    'formal_p95':'PENDING; established TASK-087 minimum is 60 warm requests',
                    'diagnostic_subset':args.warm_requests < 60,
                    'between_requests':'Existing clearClarification UI action ends the prior working goal; no memory/rules/state injection, no model restart.'}
            report['warm_requests'] = warm
            warm_pid = terminal['server_process_id']
            warm_source = call('warm.source_before', source, 'GetItemCount', {'ItemId':'wood'}).get('ReturnValue')
            warm_csv = Path(saved)/'Profiling/CSV'/('Task102Warm-'+pool+'.csv')
            warm['csv'] = str(warm_csv)
            for command in ['CsvProfile STARTFILE=Task102Warm-'+pool, 'CsvProfile START']:
                call('warm.csv_start', system, 'ExecuteConsoleCommand',
                     {'WorldContextObject':natural['pawn'], 'SpecificPlayer':natural['controller'], 'Command':command})
            csv_started = True
            warm['capture_start_monotonic'] = time.monotonic()
            for index in range(args.warm_requests):
                sample = {'index':index+1}
                warm['samples'].append(sample)
                sample['ready_before'] = call('warm.ready', ai, 'IsModelReady').get('ReturnValue')
                sample['server_pid'] = call('warm.pid', ai, 'GetServerProcessId').get('ReturnValue')
                if sample['ready_before'] is not True or sample['server_pid'] != warm_pid:
                    sample['result'] = 'FAILED_MODEL_NOT_WARM'
                    break
                sample['end_previous_goal'] = call('warm.end_previous_goal', natural['screen'], 'ExecuteAction', {'Action':'clearClarification'}).get('ReturnValue')
                if sample['end_previous_goal'] is not True:
                    sample['result'] = 'FAILED_END_PREVIOUS_GOAL'
                    break
                sample['clock_before'] = call('warm.clock_before', clock, 'GetSnapshot').get('ReturnValue')
                started = time.monotonic()
                accepted = call('warm.submit', ai, 'SubmitPlayerText',
                    {'Speaker':natural['pawn'],'Companion':companion,'Text':probe['text']}).get('ReturnValue')
                sample['accepted'] = accepted
                if accepted is not True:
                    sample['result'] = 'FAILED_SUBMISSION'
                    break
                while call('warm.busy', ai, 'IsBusy').get('ReturnValue') is True:
                    if time.monotonic()-started > 125:
                        raise TimeoutError('Warm request exceeded the existing inference timeout plus observation allowance')
                    time.sleep(.5)
                sample['terminal_observed_seconds'] = time.monotonic()-started
                sample['submission_to_http_response_seconds'] = call('warm.http_latency', ai, 'GetLastLatencySeconds').get('ReturnValue')
                sample['reason'] = call('warm.reason', ai, 'GetReasonCode').get('ReturnValue')
                sample['raw'] = call('warm.raw', ai, 'GetLastStructuredResult').get('ReturnValue')
                sample['candidate'] = call('warm.candidate', ai, 'GetCandidate').get('ReturnValue')
                sample['has_candidate'] = call('warm.has_candidate', ai, 'HasCandidate').get('ReturnValue')
                sample['generation_calls'] = call('warm.generations', ai, 'GetGenerationCalls').get('ReturnValue')
                sample['input_tokens'] = call('warm.input_tokens', ai, 'GetInputTokens').get('ReturnValue')
                sample['output_tokens'] = call('warm.output_tokens', ai, 'GetOutputTokens').get('ReturnValue')
                sample['filtered_context'] = call('warm.filtered_context', ai, 'GetLastFilteredContext').get('ReturnValue')
                sample['dialogue_reply'] = call('warm.dialogue_reply', natural['screen'], 'GetCurrentDialogueReply').get('ReturnValue')
                sample['dialogue_getter_observed_seconds'] = time.monotonic()-started
                sample['clock_after'] = call('warm.clock_after', clock, 'GetSnapshot').get('ReturnValue')
                candidate = sample['candidate'] if isinstance(sample['candidate'],dict) else {}
                sample['result'] = 'PASS' if (not sample['reason'] and sample['has_candidate'] is True
                    and sample['generation_calls'] == 1 and str(candidate.get('Intent','')).casefold() == 'collect'
                    and str(candidate.get('Item','')).casefold() == 'wood' and candidate.get('Quantity') == 1
                    and candidate.get('QuantityMode') == 'additional_acquired' and candidate.get('SourceRef') == 'S1') else 'FAIL'
                write_report()
            warm['capture_stop_requested_monotonic'] = time.monotonic()
            call('warm.csv_stop', system, 'ExecuteConsoleCommand',
                 {'WorldContextObject':natural['pawn'], 'SpecificPlayer':natural['controller'], 'Command':'CsvProfile STOP'})
            csv_started = False
            warm_csv_deadline = min(deadline, time.monotonic()+30)
            while True:
                ensure_running()
                try:
                    warm_content = warm_csv.read_text(encoding='utf-8-sig') if warm_csv.exists() else ''
                except PermissionError:
                    warm_content = ''
                if '[hasheaderrowatend]' in warm_content.lower():
                    break
                if time.monotonic() >= warm_csv_deadline:
                    raise TimeoutError('Warm CSV has no complete final header; no partial-frame credit')
                time.sleep(.5)
            warm_timings, warm_metadata = parse_frame_timings(warm_content)
            warm_frames = {**frame_statistics(warm_timings), 'dropped_frames':0, 'frame_exclusions':[],
                           'scope':'All frames from before the first warm request through the final observed terminal response, including UI queries and inter-request goal clearing. No execution or viewport first-paint credit.'}
            warm_frames['diagnostic_frame_target_pass'] = (warm_frames['p99_ms'] <= 16.67
                and warm_frames['one_percent_low_fps'] >= 60 and warm_frames['over_50ms_fraction'] <= .001)
            warm_frames['csv_metadata'] = {k.lower():v for k,v in warm_metadata.items() if k.lower() != 'commandline'}
            warm['frame_report'] = warm_frames
            (out/'warm-frame-report.json').write_text(json.dumps(warm_frames,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
            warm['unconfirmed_source_unchanged'] = call('warm.source_after', source, 'GetItemCount', {'ItemId':'wood'}).get('ReturnValue') == warm_source
            warm['semantic_failures'] = sum(s.get('result')!='PASS' for s in warm['samples'])
            complete = len(warm['samples']) == args.warm_requests and all('dialogue_getter_observed_seconds' in s for s in warm['samples'])
            if complete and warm['unconfirmed_source_unchanged']:
                # Nearest-rank p95 of all completed samples, including semantic refusals.
                rank = (95*len(warm['samples'])+99)//100-1
                for field in ('submission_to_http_response_seconds','terminal_observed_seconds','dialogue_getter_observed_seconds'):
                    values = [s[field] for s in warm['samples']]
                    warm[field+'_p95'] = sorted(values)[rank] if all(v is not None and v>0 for v in values) else None
                warm['threshold_seconds'] = 10 if args.backend == 'vulkan' else 30
                warm['terminal_latency_target_pass'] = warm['terminal_observed_seconds_p95'] <= warm['threshold_seconds']
                warm['formal_p95'] = ('COMPONENT_WINDOW_ONLY; viewport paint NOT_RUN' if len(warm['samples'])>=60
                                      else 'INSUFFICIENT_SAMPLES; diagnostic quantiles only')
                report['warm_p95'] = warm['formal_p95']
            else:
                report['warm_p95'] = 'INCOMPLETE; failures retained, no p95'
                warm['formal_p95'] = report['warm_p95']
        report.update(status='STABLE_CAPTURE_COMPLETE' if probe['model_reply_received'] else 'CAPTURE_COMPLETE_MODEL_FAILED',stage='complete')
    except Exception as exc:
        status = ('FAILED_STARTUP' if capture_observed_at is None else 'FAILED_RUNTIME') if report.get('runtime_failure_kind') else 'FAILED'
        report.update(status=status,error=repr(exc))
    finally:
        cleanup = True
        deadline = overall_deadline
        context = report['references'].get('natural') or report['references'].get('bootstrap')
        if context and pid and process_running(pid) and not report.get('runtime_failure_kind'):
            if csv_started:
                try:
                    call('cleanup.csv_stop',system,'ExecuteConsoleCommand',{'WorldContextObject':context['pawn'],'SpecificPlayer':context['controller'],'Command':'CsvProfile STOP'})
                except Exception as exc:
                    report['cleanup_csv_stop_error'] = repr(exc)
            try:
                report['normal_quit_return'] = call('cleanup.normal_quit',context['screen'],'ExecuteAction',{'Action':'quit'})
            except Exception as exc:
                report['normal_quit_error'] = repr(exc)
            time.sleep(.5)
        stopped = {'ok':False,'reason':'No owned launched PID'}
        if pid:
            try:
                stopped = ue.runtime.stop_editor(pid)
            except Exception as exc:
                stopped = {'ok':False,'process_id':pid,'error':repr(exc)}
        (out/'stop.json').write_text(json.dumps(stopped,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        report['stop_request_ok'] = bool(stopped.get('ok'))
        report['process_exit_verified'] = not process_running(pid) if pid else None
        if log.is_file():
            report['model_log_diagnostic'] = analyze_model_log(log.read_text(encoding='utf-8-sig'))
        if old_model_log is None:
            os.environ.pop('LLAMA_ARG_LOG_FILE',None)
        else:
            os.environ['LLAMA_ARG_LOG_FILE'] = old_model_log
        write_report()
        print(json.dumps({key:report.get(key) for key in ['run','output','process_id','status','stage','model_request_count','runtime_failure_kind','error','six_scene_joint_matrix','process_exit_verified']},ensure_ascii=False),flush=True)
    return 0 if report['status'] == 'STABLE_CAPTURE_COMPLETE' and report['process_exit_verified'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
