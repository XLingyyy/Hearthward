"""Freeze only the game's owned model child through the documented Windows debug API; exercise real HTTP timeout."""
import ctypes,time,json,unreal
from pathlib import Path

def scenarios(check,wait,delay,w,pc,ui,p,c,a,s,store):
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.DebugActiveProcess.argtypes=[ctypes.c_ulong];kernel.DebugActiveProcess.restype=ctypes.c_int
    kernel.DebugActiveProcessStop.argtypes=[ctypes.c_ulong];kernel.DebugActiveProcessStop.restype=ctypes.c_int
    kernel.DebugSetProcessKillOnExit.argtypes=[ctypes.c_int];kernel.DebugSetProcessKillOnExit.restype=ctypes.c_int
    elapsed=[]
    for i in range(3):
        prefix=f'R{i+1}_'
        check(prefix+'warm',a.submit_player_text(p,c,'仓库有多少木材？'));yield wait(lambda:not a.is_busy())
        check(prefix+'ready',a.is_model_ready())
        check(prefix+'submit',a.submit_player_text(p,c,'采集两份木材带回营地。'))
        yield wait(lambda:a.is_busy() and a.get_input_tokens()>0,20)
        pid=a.get_server_process_id();check(prefix+'owned_process',pid>0)
        attached=bool(kernel.DebugActiveProcess(pid));check(prefix+'debug_attach',attached)
        try:
            check(prefix+'debug_does_not_own_lifetime',bool(kernel.DebugSetProcessKillOnExit(False)))
            start=time.monotonic();yield wait(lambda:not a.is_busy(),145)
            elapsed.append(time.monotonic()-start)
            check(prefix+'actual_120s_deadline',115<=elapsed[-1]<=140)
            check(prefix+'timeout_safe',not a.has_candidate() and c.get_requested()==0 and store.get_item_count('wood')==0 and a.get_reason_code()=='MODEL_UNAVAILABLE')
            check(prefix+'no_automatic_restart',a.get_server_process_id()==0)
        finally:
            # Timeout may already have terminated this owned child. Detach never targets another PID.
            kernel.DebugActiveProcessStop(pid)
        check(prefix+'deterministic_inventory',a.query_inventory(p,c,'wood') and a.get_server_process_id()==0)
    check('explicit_recovery',a.submit_player_text(p,c,'营地木材库存是多少？'));yield wait(lambda:not a.is_busy())
    check('recovered',a.is_model_ready() and a.get_last_applied_intent()=='inventory')
    (Path(unreal.Paths.project_dir())/'docs/qa/evidence/TASK-025/rev2/http-timeout-seconds.json').write_text(json.dumps({'seconds':elapsed,'injection':'DebugActiveProcess on LocalAISubsystem owned PID only; game/UI remains live','reference':'https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-debugactiveprocess'}),encoding='utf-8')
