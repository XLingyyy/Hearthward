"""One isolated standalone prologue diagnostic. Model input is operated through the normal UI.
This does not establish the six-scene joint matrix or a warm-request p95.
"""
import argparse
import csv
from datetime import datetime, timedelta
import io
import json
import math
import re
import subprocess
import sys
import time
from pathlib import Path
from uuid import uuid4


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


parser = argparse.ArgumentParser()
parser.add_argument('--backend', choices=['cpu', 'vulkan'], required=True)
parser.add_argument('--run', required=True)
parser.add_argument('--frames', type=int, default=9000)
args = parser.parse_args()
root = Path(__file__).resolve().parents[3]
factory = next(p for p in root.parents if (p/'engine_adapters/ue5/__init__.py').is_file())
sys.path.insert(0, str(factory))
from engine_adapters.ue5 import UEClient

out = root/'.agent-local/qa/TASK-102'/args.run
out.mkdir(parents=True, exist_ok=False)
log = out/'runtime.log'
ue = UEClient(project_path=str(root/'Hearthward.uproject'), ue_root='G:/UnrealEngine/UE_5.8', port=30201, runtime_port=30202)
commands = [f'{60+i}:sg.{name}Quality 3' for i, name in enumerate([
    'ViewDistance', 'AntiAliasing', 'Shadow', 'GlobalIllumination', 'Reflection',
    'PostProcess', 'Texture', 'Effects', 'Foliage', 'Shading', 'Landscape'])]
commands += ['71:r.ScreenPercentage 100', '72:r.DynamicRes.OperationMode 0', '73:t.MaxFPS 0',
             '74:r.VSync 0', '75:r.AntiAliasingMethod 2', '76:r.Shadow.Virtual.Enable 1', '77:r.Nanite 1']
launch = ue.runtime.launch_editor(
    map_path='/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds?game=/Script/Hearthward.HearthwardGameMode?HearthwardNewGame=1',
    extra_args=['-game', '-dx12', '-windowed', '-ResX=1920', '-ResY=1080', '-ForceRes', '-NoVSync',
                '-UserDir='+str(out/'profile'), '-HearthwardSaveTestPool='+str(uuid4()),
                '-HearthwardAIBackend='+args.backend, '-abslog='+str(log), '-ModelContextProtocolPort=8001',
                '-csvCaptureFrames='+str(args.frames), '-csvCompression=0', '-csvGpuStats', '-ExitAfterCsvProfiling',
                '-csvExecCmds='+','.join(commands)])
(out/'launch.json').write_text(json.dumps(launch, ensure_ascii=False, indent=2), encoding='utf-8')
pid = launch.get('payload', {}).get('process_id')
print(json.dumps({'ok':launch['ok'], 'pid':pid, 'output':str(out)}, ensure_ascii=False), flush=True)
if not launch['ok']:
    sys.exit(1)
try:
    deadline = time.monotonic()+600
    last_resource = 0
    captured = None
    while time.monotonic() < deadline:
        if time.monotonic()-last_resource >= 10:
            last_resource = time.monotonic()
            source = log.read_text(encoding='utf-8-sig', errors='strict') if log.exists() else ''
            model_pids = [int(x) for x in re.findall(r'Local AI runtime started pid=(\d+)', source)]
            process_filter = ' OR '.join(f'ProcessId={x}' for x in [pid]+model_pids)
            command = '@{Processes=@(Get-CimInstance Win32_Process -Filter "'+process_filter+'" | Select-Object ProcessId,WorkingSetSize,PageFileUsage);Memory=(Get-CimInstance Win32_OperatingSystem | Select-Object FreePhysicalMemory,TotalVisibleMemorySize,TotalVirtualMemorySize,FreeVirtualMemory)} | ConvertTo-Json -Depth 3 -Compress'
            process = subprocess.run(['powershell', '-NoProfile', '-Command', command], capture_output=True, text=True, encoding='utf-8')
            gpu = subprocess.run(['nvidia-smi', '--query-gpu=timestamp,temperature.gpu,power.draw,memory.used,utilization.gpu', '--format=csv,noheader'], capture_output=True, text=True, encoding='utf-8')
            with (out/'resources.jsonl').open('a', encoding='utf-8') as stream:
                stream.write(json.dumps({'time':time.time(), 'model_pids':model_pids, 'ue_process':process.stdout.strip(),
                                         'gpu':gpu.stdout.strip(), 'errors':[x.strip() for x in [process.stderr, gpu.stderr] if x.strip()]})+'\n')
        for candidate in (out/'profile/Saved/Profiling/CSV').glob('*.csv'):
            try:
                content = candidate.read_text(encoding='utf-8-sig')
            except PermissionError:
                continue
            if '[HasHeaderRowAtEnd]' in content:
                captured = candidate
                break
        if captured:
            break
        time.sleep(2)
    if not captured:
        raise TimeoutError('Engine CSV profiling did not finish within 600 seconds')
    timings, metadata = parse_frame_timings(captured.read_text(encoding='utf-8-sig'))
    model = analyze_model_log(log.read_text(encoding='utf-8-sig', errors='strict'))
    result = {'scene':'prologue', 'backend':args.backend, 'source_head':'6fcf5c22e965f0f7409438f19bc7b09e96ffb058',
              'local_uncommitted_changes':True, 'evidence_level':'standalone boot-capture diagnostic; all captured frames retained',
              'csv':str(captured), **frame_statistics(timings), 'csv_metadata':metadata,
              'frame_exclusions':[], 'requested_render':[1920,1080,100,3,'DX12/SM6/TAA/Nanite/VSM'],
              'configuration_scope':'Startup frames precede render commands at frames 60-77; no settled-scene PASS',
              'configuration_verified':'Check runtime log and captured normal UI separately',
              'model_log_diagnostic':model, 'joint_matrix_pass':'NOT_EVALUATED', 'warm_p95':'NOT_RUN'}
    (out/'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False), flush=True)
finally:
    print(json.dumps(ue.runtime.stop_editor(pid), ensure_ascii=False), flush=True)
