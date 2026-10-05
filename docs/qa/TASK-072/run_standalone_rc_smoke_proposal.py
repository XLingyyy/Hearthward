"""Root-owned public-reference smoke, with optional one-request engine CSV diagnosis."""
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

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--project', type=Path, default=Path('G:/GameFactory/Hearthward/.agent-local/task051/Hearthward.uproject'))
parser.add_argument('--ue-root', type=Path, default=Path('G:/UnrealEngine/UE_5.8'))
parser.add_argument('--label', default='standalone-public-reference-smoke')
parser.add_argument('--packaged-executable', type=Path, help='Use an existing archived Game executable through public UEClient runtime ownership.')
parser.add_argument('--packaged-configuration', choices=['Development', 'Shipping'], default='Development')
parser.add_argument('--timeout', type=int, default=240)
parser.add_argument('--model-csv', action='store_true', help='Opt in to one normal-new collect request and at least 60 s of engine CSV frames.')
parser.add_argument('--backend', choices=['cpu', 'vulkan'], default='cpu')
parser.add_argument('--bundle', type=Path, default=Path('G:/GameFactory/Hearthward/Runtime/LocalAI'))
save_group = parser.add_mutually_exclusive_group()
save_group.add_argument('--shipping-save-write', action='store_true', help='Write one public manual save in a fresh isolated Shipping UserDir.')
save_group.add_argument('--shipping-save-read', type=Path, help='Read a successful writer results.json and Continue its same owned Shipping UserDir in a new process.')
args = parser.parse_args()
if not 0 < args.timeout <= 300:
    parser.error('--timeout must be 1..300 seconds')
project = args.project.resolve()
if not project.is_file():
    parser.error('--project must name an existing .uproject')
if args.packaged_executable:
    args.packaged_executable = args.packaged_executable.resolve()
    if not args.packaged_executable.is_file():
        parser.error('--packaged-executable must name an existing Game executable')
if args.packaged_configuration == 'Shipping' and (not args.packaged_executable or args.model_csv):
    parser.error('Shipping requires an actual packaged executable and reference-only mode; its CSV profiler is compiled out.')
shipping_save_phase = 'write' if args.shipping_save_write else 'read' if args.shipping_save_read else None
if shipping_save_phase and (args.packaged_configuration != 'Shipping' or not args.packaged_executable or args.model_csv):
    parser.error('Shipping save phases require a packaged Shipping executable and no model/CSV mode.')
save_writer = None
if args.shipping_save_read:
    args.shipping_save_read = args.shipping_save_read.resolve()
    save_writer = json.loads(args.shipping_save_read.read_text(encoding='utf-8-sig'))
    writer_pool = str(uuid.UUID(save_writer['pool']))
    writer_output = (project.parent/'Saved/Task072'/('standalone-rc-smoke-'+writer_pool)).resolve()
    if args.shipping_save_read != writer_output/'results.json' or Path(save_writer['output']).resolve() != writer_output:
        parser.error('Reader source must be the exact successful writer results in this project owned Task072 directory.')
    if save_writer.get('status') != 'SHIPPING_SAVE_WRITE_PASS' or save_writer.get('packaged_configuration') != 'Shipping' or not save_writer.get('stop_request_ok'):
        parser.error('Reader requires a completed successful Shipping save writer.')
    if Path(save_writer['packaged_executable']).resolve() != args.packaged_executable:
        parser.error('Reader must use the same archived Shipping executable as its writer.')
    writer_user = writer_output/'User'
    if Path(save_writer['shipping_save_roundtrip']['user_dir']).resolve() != writer_user or not (writer_user/'Saved/SaveGames/HearthwardPrototype/Compatible-v8/pool.hws').is_file():
        parser.error('Writer UserDir or its actual pool file is missing/mismatched.')
if args.model_csv:
    args.bundle = args.bundle.resolve()
    for required in [args.bundle/'models/Qwen3.5-4B-Q4_K_M.gguf', args.bundle/'bin'/args.backend/'llama-server.exe']:
        if not required.is_file():
            parser.error('Prepared existing bundle file is missing: '+str(required))
configured = os.environ.get('HEARTHWARD_FACTORY_ROOT')
candidates = [Path(configured)] if configured else []
candidates.extend(project.parent.parents)
factory = next((p for p in candidates if (p/'engine_adapters/ue5/__init__.py').is_file()), None)
if factory is None:
    parser.error('Set HEARTHWARD_FACTORY_ROOT to the prepared GameFactory checkout.')
sys.path.insert(0, str(factory.resolve()))
from engine_adapters.ue5 import UEClient

pool = str(uuid.uuid4())
out = project.parent/'Saved/Task072'/('standalone-rc-smoke-'+pool)
out.mkdir(parents=True, exist_ok=False)
user_dir = writer_user if save_writer is not None else out/'User'
with socket.socket() as port_probe:
    port_probe.bind(('127.0.0.1', 0))
    port = port_probe.getsockname()[1]
ue = UEClient(project_path=project, ue_root=args.ue_root, host='127.0.0.1', port=port)
opener = request.build_opener(request.ProxyHandler({}))
overall_deadline = time.monotonic()+args.timeout
deadline = overall_deadline-(10 if args.model_csv or shipping_save_phase else 0)
pid = None
csv_started = False
report = {'pool': pool, 'label': args.label, 'output': str(out), 'port': port,
          'status': 'NOT_RUN', 'stage': 'launch', 'timeout_seconds': args.timeout,
          'method': 'UEClient-owned UnrealEditor -game / normal Bootstrap ExecuteAction new / public RC UFUNCTIONs and HUD.Screen READ_ACCESS',
          'acceptance': 'NOT_EVALUATED', 'performance': 'NOT_RUN', 'model_request_count': 0,
          'model_parameter_changes': False, 'actor_position_changes': False,
          'bootstrap_seed_attempt': None, 'references': {}, 'observations': {}}
if args.packaged_executable:
    report['method'] = 'UEClient-owned packaged Game '+args.packaged_configuration+' / normal Bootstrap ExecuteAction new / public RC UFUNCTIONs and HUD.Screen READ_ACCESS'
    report['packaged_executable'] = str(args.packaged_executable)
    report['packaged_configuration'] = args.packaged_configuration
if shipping_save_phase:
    report['shipping_save_roundtrip'] = {'phase': shipping_save_phase, 'user_dir': str(user_dir)}
    if save_writer is not None:
        report['method'] = report['method'].replace('ExecuteAction new', 'ExecuteAction continue')
        report['shipping_save_roundtrip'].update(writer_results=str(args.shipping_save_read), writer_pid=save_writer['process_id'])


def write_report():
    (out/'results.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


def http(name, route, body=None):
    if time.monotonic() >= deadline:
        raise TimeoutError('Smoke deadline reached at '+name)
    started = time.monotonic()
    entry = {'name': name, 'stage': report['stage'], 'method': 'GET' if body is None else 'PUT',
             'route': route, 'request': body, 'started_monotonic': started}
    try:
        req = request.Request('http://127.0.0.1:'+str(port)+route,
                              data=None if body is None else json.dumps(body).encode('utf-8'),
                              headers={'Content-Type': 'application/json'}, method=entry['method'])
        with opener.open(req, timeout=min(10, max(.1, deadline-started))) as response:
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
    '/Script/Engine.KismetSystemLibrary': ['IsValid', 'GetOuterObject', 'GetPathName'],
    '/Script/Engine.GameplayStatics': ['GetPlayerPawn', 'GetCurrentLevelName', 'GetPlayerController', 'GetGameInstance', 'GetAllActorsOfClass'],
    '/Script/Hearthward.HearthwardCharacter': ['GetLevel'],
    '/Script/Engine.PlayerController': ['GetHUD'],
    '/Script/Engine.SubsystemBlueprintLibrary': ['GetGameInstanceSubsystem', 'GetWorldSubsystem'],
    '/Script/Hearthward.HearthwardScreenWidget': ['GetPage', 'ExecuteAction'],
    '/Script/Hearthward.HearthwardLoadingSubsystem': ['IsLoading'],
    '/Script/Hearthward.HearthwardSaveSubsystem': ['IsNaturalWorldEnabled', 'GetCampaignId'],
    '/Script/Hearthward.HearthwardLocalAISubsystem': ['IsBusy', 'IsModelReady', 'GetGenerationCalls', 'GetServerProcessId'],
}
if args.model_csv:
    allowed_calls['/Script/Engine.KismetSystemLibrary'] += ['ExecuteConsoleCommand', 'GetProjectSavedDirectory', 'GetConsoleVariableStringValue']
    allowed_calls['/Script/Engine.GameplayStatics'] += ['IsGamePaused']
    allowed_calls['/Script/Hearthward.HearthwardCompanionFixture'] = ['CanCommunicate']
    allowed_calls['/Script/Hearthward.HearthwardInventoryComponent'] = ['GetItemCount']
    allowed_calls['/Script/Hearthward.HearthwardLocalAISubsystem'] += ['SubmitPlayerText', 'GetStatus', 'GetLastInput', 'GetLastStructuredResult', 'GetLastLatencySeconds', 'GetInputTokens', 'GetOutputTokens', 'GetReasonCode', 'GetLastAppliedIntent', 'HasCandidate', 'GetCandidate', 'GetLastFilteredContext']
if args.packaged_configuration == 'Shipping':
    allowed_calls['/Script/Engine.KismetSystemLibrary'].append('GetProjectSavedDirectory')
if shipping_save_phase:
    allowed_calls['/Script/Hearthward.HearthwardSaveSubsystem'] += ['GetPoints', 'LoadPointIndex', 'GetStatus', 'GetSafetyDescription'] + (['SavePoint'] if shipping_save_phase == 'write' else [])
policy_ini = out/'RemoteControl.ini'
policy_lines = ['[/Script/RemoteControlCommon.RemoteControlSettings]',
                'bAllowAnyRemoteFunctionCall=False', 'bAutoStartWebSocketServer=False',
                'RemoteControlHttpServerPort='+str(port)]
if args.model_csv:
    policy_lines.append('bAllowConsoleCommandRemoteExecution=True')
if args.packaged_configuration == 'Shipping':
    policy_lines.append('bAutoStartWebServer=True')
policy_lines += [
    'CustomAllowedRemoteFunctionCalls=(ClassPath="'+cls+'",FunctionName=("'+function+'"),bAllowChildClasses=False)'
    for cls, functions in allowed_calls.items() for function in functions
]
policy_ini.write_text('\n'.join(policy_lines)+'\n', encoding='utf-8')
allow_args = ['-RemoteControlINI='+str(policy_ini)]
report['remote_function_policy'] = {'allow_any': False, 'exact_rules': allowed_calls, 'child_classes': False,
                                    'scope': 'owned Saved session ini only', 'file': str(policy_ini)}
try:
    if args.model_csv:
        os.environ['LLAMA_ARG_LOG_FILE'] = str(out/'local-ai.log')
        report['model_server_log_file'] = str(out/'local-ai.log')
    launch_args = [
        '-game', '-RCWebControlEnable', '-HearthwardSaveTestPool='+pool,
        '-ini:RemoteControl:[/Script/RemoteControlCommon.RemoteControlSettings]:RemoteControlHttpServerPort='+str(port),
        '-ini:RemoteControl:[/Script/RemoteControlCommon.RemoteControlSettings]:bAutoStartWebSocketServer=False',
        '-UserDir='+str(user_dir), '-AbsLog='+str(out/'engine.log'),
        '-windowed', '-ResX=1920', '-ResY=1080', '-NoSplash']+allow_args+([
            '-dx12', '-ForceRes', '-NoVSync', '-csvCompression=0',
            '-HearthwardAIBackend='+args.backend, '-HearthwardAIGpuLayers=16',
            '-HearthwardAIBundlePath='+str(args.bundle)] if args.model_csv else [])
    if args.packaged_executable:
        launch = ue.runtime.launch_packaged(args.packaged_executable, extra_args=[
            '/Game/Hearthward/Bootstrap/L_Bootstrap'] + ([] if args.packaged_configuration == 'Shipping' else [
            '-ExecCmds=WebControl.StartServer '+str(port)]) + launch_args)
    else:
        launch = ue.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap', extra_args=launch_args)
    pid = launch.get('payload', {}).get('process_id')
    (out/'launch.json').write_text(json.dumps(launch, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    report.update(process_id=pid, status='RUNNING')
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
    report['stage'] = 'check_exact_function_rule'
    try:
        call('rc.unlisted_readonly_control', system, 'IsDedicatedServer', {'WorldContextObject': boot['pawn']})
    except error.HTTPError as exc:
        if exc.code != 400:
            raise
        last_event = json.loads((out/'http-events.jsonl').read_text(encoding='utf-8-sig').splitlines()[-1])
        if 'not allowed by remote control settings' not in last_event.get('response_text', ''):
            raise RuntimeError('Unlisted readonly function failed for a different reason')
        report['remote_function_policy']['unlisted_readonly_control'] = 'EXPECTED_400_FUNCTION_NOT_ALLOWED'
    else:
        raise RuntimeError('Unlisted readonly function was allowed; exact function policy not verified')
    instance = ref(call('bootstrap.game_instance', gameplay, 'GetGameInstance',
                        {'WorldContextObject': boot['pawn']}).get('ReturnValue'), 'bootstrap.game_instance')
    report['references']['game_instance'] = instance
    if args.packaged_configuration == 'Shipping':
        actual_saved = Path(call('shipping.isolated_saved_directory', system, 'GetProjectSavedDirectory').get('ReturnValue')).resolve()
        expected_saved = (user_dir/'Saved').resolve()
        if actual_saved != expected_saved:
            raise RuntimeError('Shipping save directory is not the owned UserDir: '+str(actual_saved))
        report['shipping_save_isolation'] = {'actual': str(actual_saved), 'expected': str(expected_saved), 'verified_before_new': shipping_save_phase != 'read', 'verified_before_action': 'continue' if shipping_save_phase == 'read' else 'new', 'test_pool_parameter_credit': False}
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
    if shipping_save_phase == 'read':
        report['stage'] = 'shipping_save_read_before_continue'
        boot_save = ref(call('bootstrap.save', subsystems, 'GetWorldSubsystem',
                             {'ContextObject': boot['pawn'], 'Class': '/Script/Hearthward.HearthwardSaveSubsystem'}).get('ReturnValue'), 'bootstrap.save')
        if call('bootstrap.saved_index', boot_save, 'LoadPointIndex').get('ReturnValue') is not True:
            raise RuntimeError('Shipping Bootstrap could not read the real writer pool: '+str(call('bootstrap.save_status', boot_save, 'GetStatus').get('ReturnValue')))
        boot_points = call('bootstrap.saved_points', boot_save, 'GetPoints').get('ReturnValue')
        expected = save_writer['shipping_save_roundtrip']
        if not isinstance(boot_points, list):
            raise RuntimeError('Public GetPoints did not return a save-point array')
        ticks = [p.get('Created', {}).get('Ticks') for p in boot_points]
        if not ticks or any(not isinstance(t, int) for t in ticks):
            raise RuntimeError('Actual GetPoints Created did not expose reflected FDateTime.Ticks')
        latest = [p for p in boot_points if p['Created']['Ticks'] == max(ticks)]
        if len(latest) != 1 or latest[0].get('SaveId') != expected['save_id'] or latest[0].get('Manual') is not True or latest[0].get('CampaignId') != expected['campaign_id'] or latest[0].get('Created') != expected['created']:
            raise RuntimeError('Actual disk-loaded unique latest manual point differs from the writer')
        report['shipping_save_roundtrip']['bootstrap_latest_manual_point_found'] = True
    entry_action = 'continue' if shipping_save_phase == 'read' else 'new'
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
    if call('natural.enabled', save, 'IsNaturalWorldEnabled').get('ReturnValue') is not True:
        raise RuntimeError('Formal natural-world session is not enabled')
    report['observations']['campaign_id'] = call('natural.campaign_id', save, 'GetCampaignId').get('ReturnValue')
    for field, function in [('busy', 'IsBusy'), ('ready', 'IsModelReady'), ('generation_calls', 'GetGenerationCalls'), ('server_process_id', 'GetServerProcessId')]:
        report['observations'][field] = call('ai.'+field, ai, function).get('ReturnValue')
    if report['observations']['busy'] is not False or report['observations']['ready'] is not False or report['observations']['generation_calls'] != 0 or report['observations']['server_process_id'] != 0:
        raise RuntimeError('Fresh reference-only smoke observed unexpected model activity')
    report.update(status='REFERENCE_SMOKE_PASS', stage='complete')
    if shipping_save_phase:
        report['stage'] = 'shipping_save_'+shipping_save_phase
        probe = report['shipping_save_roundtrip']
        points = call('shipping.current_points', save, 'GetPoints').get('ReturnValue')
        if not isinstance(points, list):
            raise RuntimeError('Shipping public GetPoints did not return an array')
        if shipping_save_phase == 'write':
            if call('shipping.manual_save', save, 'SavePoint', {'Manual': True}).get('ReturnValue') is not True:
                probe['save_status'] = call('shipping.save_status', save, 'GetStatus').get('ReturnValue')
                probe['save_safety'] = call('shipping.save_safety', save, 'GetSafetyDescription').get('ReturnValue')
                raise RuntimeError('Public Shipping SavePoint(true) rejected the current normal world')
            after_points = call('shipping.saved_points', save, 'GetPoints').get('ReturnValue')
            if not isinstance(after_points, list):
                raise RuntimeError('Public GetPoints did not return an array after saving')
            added = [p for p in after_points if not any(p.get('SaveId') == old.get('SaveId') for old in points)]
            if len(added) != 1 or added[0].get('Manual') is not True or added[0].get('CampaignId') != report['observations']['campaign_id']:
                raise RuntimeError('Public save did not append one actual manual point in this campaign')
            point = added[0]
            ticks = [p.get('Created', {}).get('Ticks') for p in after_points]
            if not ticks or any(not isinstance(t, int) for t in ticks) or sum(t == max(ticks) for t in ticks) != 1 or point['Created']['Ticks'] != max(ticks):
                raise RuntimeError('New manual point is not the unique actual latest point selected by Continue')
            pool_file = user_dir/'Saved/SaveGames/HearthwardPrototype/Compatible-v8/pool.hws'
            if not pool_file.is_file() or pool_file.stat().st_size == 0:
                raise RuntimeError('Public Shipping save did not create the real isolated pool file')
            probe.update(save_id=point['SaveId'], campaign_id=point['CampaignId'], created=point['Created'],
                         point_count_before=len(points), point_count_after=len(after_points),
                         pool_file=str(pool_file), pool_bytes=pool_file.stat().st_size)
            report.update(status='SHIPPING_SAVE_WRITE_PASS', stage='complete')
        else:
            expected = save_writer['shipping_save_roundtrip']
            persisted = [p for p in points if p.get('SaveId') == expected['save_id']]
            restored_status = call('shipping.restored_status', save, 'GetStatus').get('ReturnValue')
            probe['restored_status'] = restored_status
            if report['observations']['campaign_id'] != expected['campaign_id'] or len(persisted) != 1 or persisted[0].get('Manual') is not True or restored_status != '世界与知识已恢复；旧时间线请求已废止':
                raise RuntimeError('Normal Shipping Continue did not successfully restore the campaign selected from the actual latest manual point')
            probe.update(save_id=expected['save_id'], campaign_id=expected['campaign_id'],
                         point_count=len(points), reader_pid=pid, restored_after_normal_continue=True,
                         current_save_id_getter='NOT_EXPOSED; point selection verified through unique latest GetPoints and normal Continue source path',
                         inventory_verification='NOT_RUN')
            report.update(status='SHIPPING_SAVE_READ_PASS', stage='complete')
    if args.model_csv:
        report['stage'] = 'model_csv_preconditions'
        probe = {'scene': 'normal_new_initial_scene_dialogue_fixed_view', 'backend_requested': args.backend,
                 'bundle': str(args.bundle), 'gpu_layers_requested': 16, 'public_submit_calls': 0,
                 'text': '采集1份木材送入营地仓库', 'csv_minimum_seconds': 60,
                 'warm_p95': 'NOT_RUN', 'ttft': 'NOT_RUN', 'ui_first_paint_latency': 'NOT_RUN',
                 'process_memory_vram_available_ram': 'NOT_RUN', 'task072_acceptance': 'NOT_EVALUATED'}
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
            time.sleep(1)
        saved = call('probe.actual_saved_directory', system, 'GetProjectSavedDirectory').get('ReturnValue')
        if not isinstance(saved, str) or not Path(saved).is_absolute():
            raise RuntimeError('Public Saved directory is not an absolute path')
        csv_path = Path(saved)/'Profiling/CSV'/('Task072-'+pool+'.csv')
        if not csv_path.resolve().is_relative_to(out.resolve()):
            raise RuntimeError('UserDir did not isolate the actual CSV output under this session')
        probe['csv'] = str(csv_path)
        report['stage'] = 'model_csv_start'
        for command in ['CsvProfile STARTFILE=Task072-'+pool, 'CsvProfile START']:
            call('probe.csv_command', system, 'ExecuteConsoleCommand',
                 {'WorldContextObject': natural['pawn'], 'SpecificPlayer': natural['controller'], 'Command': command})
        csv_started = True
        file_start_deadline = min(deadline, time.monotonic()+10)
        while not csv_path.is_file():
            if time.monotonic() >= file_start_deadline:
                raise TimeoutError('CSV START returned without creating the unique capture file')
            time.sleep(.1)
        capture_observed_at = time.monotonic()
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
            if call('probe.poll.page', natural['screen'], 'GetPage').get('ReturnValue').casefold() != 'dialogue':
                raise RuntimeError('Scene page changed during sample')
            if call('probe.poll.paused', gameplay, 'IsGamePaused', {'WorldContextObject': natural['pawn']}).get('ReturnValue') is not False:
                raise RuntimeError('World became paused during joint sample')
            if terminal_observed_at is not None and observed_at-capture_observed_at >= 61:
                break
            time.sleep(1)
        probe.update(busy_observed=busy_observed,
                     cold_ready_observed_seconds=None if ready_observed_at is None else ready_observed_at-submitted_at,
                     full_ue_terminal_observed_seconds=terminal_observed_at-submitted_at,
                     poll_interval_seconds=1, model_request_count=state['generation_calls'])
        report['stage'] = 'model_csv_stop'
        call('probe.csv_stop', system, 'ExecuteConsoleCommand',
             {'WorldContextObject': natural['pawn'], 'SpecificPlayer': natural['controller'], 'Command': 'CsvProfile STOP'})
        csv_started = False
        file_end_deadline = min(deadline, time.monotonic()+30)
        while True:
            try:
                content = csv_path.read_text(encoding='utf-8-sig')
            except PermissionError:
                content = ''
            if '[hasheaderrowatend]' in content.lower():
                break
            if time.monotonic() >= file_end_deadline:
                raise TimeoutError('CSV STOP has not completed final metadata/header; no partial-file frame credit')
            time.sleep(.5)
        report['stage'] = 'model_csv_terminal_results'
        terminal = {field: call('probe.terminal.'+field, ai, function).get('ReturnValue')
                    for field, function in [('last_input', 'GetLastInput'), ('raw', 'GetLastStructuredResult'), ('submission_to_response_seconds', 'GetLastLatencySeconds'), ('input_tokens', 'GetInputTokens'), ('output_tokens', 'GetOutputTokens'), ('reason', 'GetReasonCode'), ('applied_intent', 'GetLastAppliedIntent'), ('has_candidate', 'HasCandidate'), ('candidate', 'GetCandidate'), ('filtered_context', 'GetLastFilteredContext'), ('server_process_id', 'GetServerProcessId')]}
        probe['terminal'] = terminal
        candidate = terminal['candidate'] if isinstance(terminal['candidate'], dict) else {}
        probe['expected_collect_candidate'] = (terminal['has_candidate'] is True and terminal['reason'] == '' and terminal['applied_intent'] == 'proposal'
                                                and candidate.get('Intent').casefold() == 'collect' and candidate.get('Item').casefold() == 'wood'
                                               and candidate.get('Quantity') == 1 and candidate.get('QuantityMode') == 'additional_acquired' and candidate.get('SourceRef') == 'S1')
        probe['source_wood_after'] = call('probe.source_after', source, 'GetItemCount', {'ItemId': 'wood'}).get('ReturnValue')
        probe['unconfirmed_source_unchanged'] = probe['source_wood_after'] == source_wood
        rows = list(csv.reader(content.splitlines()))
        frame_index = rows[0].index('FrameTime')
        frames = []
        for row_number, row in enumerate(rows[1:], 1):
            if not row:
                continue
            if row[0] == 'EVENTS' and row[:len(rows[0])] == rows[0]:
                continue
            if row_number == len(rows)-1 and row[:3] == ['[HasHeaderRowAtEnd]', '1', '[EventTimestamps]'] and len(row) >= 4 and row[3] in ('0', '1'):
                continue
            if len(row) <= frame_index:
                raise ValueError('Final CSV data row is missing FrameTime; no silent frame exclusion')
            value = float(row[frame_index])
            if not math.isfinite(value) or value < 0:
                raise ValueError('Final CSV has an invalid numeric FrameTime; no silent numeric frame exclusion')
            frames.append(value)
        samples = sorted(frames)
        if not samples:
            raise ValueError('Final engine CSV has no numeric FrameTime samples')
        p99 = samples[min(len(samples)-1, int((len(samples)-1)*.99))]
        slowest = samples[-max(1, len(samples)//100):]
        one_percent_low = 1000/(sum(slowest)/len(slowest))
        hitch_fraction = sum(value > 50 for value in samples)/len(samples)
        frame_report = {'total_frames': len(samples), 'dropped_frames': 0, 'measured_seconds': sum(samples)/1000,
                        'p99_frame_ms': p99, 'one_percent_low_fps': one_percent_low,
                        'frames_over_50ms': sum(value > 50 for value in samples), 'fraction_over_50ms': hitch_fraction,
                        'max_frame_ms': max(samples), 'average_fps': 1000/(sum(samples)/len(samples)),
                        'distribution': 'raw ordered engine CSV FrameTime preserved; all captured numeric rows included without numeric frame exclusions',
                        'diagnostic_frame_target_pass': p99 <= 16.67 and one_percent_low >= 60 and hitch_fraction <= .001}
        probe['frame_report'] = frame_report
        (out/'frame-report.json').write_text(json.dumps(frame_report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
        if frame_report['measured_seconds'] < 60:
            raise RuntimeError('Engine CSV contains less than 60 actual seconds; host wall time is insufficient evidence')
        if state['generation_calls'] != 1 or ready_observed_at is None or not busy_observed:
            raise RuntimeError('Joint capture did not observe exactly one actual generation with real busy/ready state')
        if terminal['last_input'] != probe['text'] or not terminal['raw']:
            raise RuntimeError('Single actual generation did not produce a parsed reply bound to this original request')
        if not probe['unconfirmed_source_unchanged']:
            raise RuntimeError('Source changed before any confirmation; preserve actual probe failure')
        report.update(status='MODEL_CSV_CAPTURE_COMPLETE', performance='ONE_SCENE_DIAGNOSTIC_ONLY', stage='complete')
except Exception as exc:
    report.update(status='FAILED', error=repr(exc))
finally:
    deadline = overall_deadline
    if (args.model_csv or shipping_save_phase) and report['references'].get('natural'):
        context = report['references']['natural']
        if csv_started:
            try:
                call('cleanup.csv_stop', system, 'ExecuteConsoleCommand',
                     {'WorldContextObject': context['pawn'], 'SpecificPlayer': context['controller'], 'Command': 'CsvProfile STOP'})
            except Exception as exc:
                report['cleanup_csv_stop_error'] = repr(exc)
        try:
            report['normal_quit_return'] = call('cleanup.normal_quit', context['screen'], 'ExecuteAction', {'Action': 'quit'})
            if shipping_save_phase and report['normal_quit_return'].get('ReturnValue') is not True:
                raise RuntimeError('Shipping public normal quit was rejected')
        except Exception as exc:
            report['normal_quit_error'] = repr(exc)
            if shipping_save_phase:
                report['status'] = 'FAILED'
                report.setdefault('error', 'Shipping normal quit did not return success: '+repr(exc))
        time.sleep(1)
    stopped = {'ok': False, 'reason': 'No owned launched PID'}
    if pid:
        try:
            stopped = ue.runtime.stop_editor(pid)
        except Exception as exc:
            stopped = {'ok': False, 'process_id': pid, 'error': repr(exc)}
    (out/'stop.json').write_text(json.dumps(stopped, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    report['stop_request_ok'] = bool(stopped.get('ok'))
    report['process_exit_verified'] = 'NOT_VERIFIED_BY_UECLIENT'
    write_report()
    print(json.dumps(report, ensure_ascii=False), flush=True)
raise SystemExit(0 if report['status'] in ('REFERENCE_SMOKE_PASS', 'MODEL_CSV_CAPTURE_COMPLETE', 'SHIPPING_SAVE_WRITE_PASS', 'SHIPPING_SAVE_READ_PASS') and report['stop_request_ok'] else 1)
