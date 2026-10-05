"""Read-only actual tokenizer probe; no inference, UE process, build or model download."""
import argparse
import json
import re
import secrets
import socket
import subprocess
import time
import urllib.request
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--project', type=Path, required=True)
    parser.add_argument('--bundle', type=Path, required=True)
    parser.add_argument('--backend', choices=('cpu', 'vulkan'), default='cpu')
    args = parser.parse_args()
    data = json.loads((args.project/'Resources/Data/gameplay.json').read_text(encoding='utf-8-sig'))
    contract = (args.project/'Source/Hearthward/AI/HearthwardAgentContract.cpp').read_text(encoding='utf-8-sig')
    runtime = (args.project/'Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp').read_text(encoding='utf-8-sig')
    items = data['items']
    stored = [x for x in items if not x.get('slot') and x.get('durability', 0) <= 0
              and x['weight'] > 0 and not x.get('uniqueClaim') and x.get('rarity') == 'common']
    policy = json.loads((args.project/'config/npc-agent.policy.json').read_text(encoding='utf-8-sig'))
    nature = data['nature']
    ids = lambda values: [x['id'] for x in values]
    resources = list(dict.fromkeys([x['item'] for x in nature['resources']]+
                                  [x['product'] for x in nature['domestic'] if x.get('product')]))
    arrays = {'All': ids(items), 'Stored': ids(stored), 'Resources': resources,
              'Recipes': ids(data['craftingRecipes']), 'Repair': ids(data['repairRecipes']),
              'People': [x['id'] for x in data['campaign']['people'] if x['id'].startswith('rescued_')],
              'Wildlife': ids(nature['wildlife']), 'Domestic': ids(nature['domestic']),
              'CampRecipes': ids(data['campEconomy']['recipes'])}
    labels = {x['id']: x['name'] for x in data['campEconomy']['recipes'] if 'name' in x}
    labels.update({x['id']: x['name'] for x in data['craftingRecipes']})
    labels.update({x['id']: x['name'] for x in items})
    labels.update({x['id']: x['name'] for x in nature['wildlife']+nature['domestic']})
    labels.update({'hold': '原地等待', 'follow': '跟随玩家', 'assist': '协助战斗', 'routine': '营地自由活动',
                   'water': '地块浇水', 'fertilize': '地块施肥', 'harvest': '成熟作物收获',
                   'deposit_feed': '栏舍喂料', 'fish': '鱼点渔获', 'none': '未指定物品'})
    for item in arrays['People']:
        labels[item] = '族人'+item[-2:]
    literal = r'((?:\\.|[^"\\])*)'
    pattern = re.compile(r'\{TEXT\("'+literal+r'"\),TEXT\("'+literal+r'"\),'
                         r'(\{[^}]*\}|\w+),(Policy\(TEXT\("\w+"\)\)|\d+),TEXT\("'+literal+r'"\)')
    rows = []
    for match in pattern.finditer(contract.split('TArray<FHearthwardAgentCapability> Result=', 1)[1].split('return Result;', 1)[0]):
        intent, description, item_expression, maximum, mode = match.groups()
        values = re.findall(r'TEXT\("([^\"]+)"\)', item_expression) if item_expression.startswith('{') else arrays[item_expression]
        maximum = int(maximum) if maximum.isdigit() else policy[re.search(r'TEXT\("(\w+)"\)', maximum).group(1)]
        rows.append((intent, json.loads('"'+description+'"'), values, maximum, mode))
    if len(rows) != 20:
        raise ValueError('current registry parser expected 20 declared capabilities')
    for intent in ('cancel', 'clarify', 'dialogue', 'refuse'):
        rows.append((intent, '取消/澄清/闲聊/拒绝；无物品参数', ['none'], 0, 'none'))
    lines = []
    for intent, description, values, maximum, mode in rows:
        names = ','.join(item+'='+labels.get(item, '未指定物品') for item in values)
        lines.append(f'{intent}: {description}；item={names}；quantity上限{maximum}；mode={mode}\n')
    describe = contract.split('FString HearthwardAgent::Describe()', 1)[1].split('FString HearthwardAgent::Schema()', 1)[0]
    intro = json.loads('"'+re.search(r'FString Out=TEXT\("'+literal+r'"\)', describe).group(1)+'"')
    outro = json.loads('"'+re.search(r'Out\+=TEXT\("'+literal+r'"\)', describe).group(1)+'"')
    system_block = runtime.split('const FString System=', 1)[1].split('TSharedPtr<FJsonObject> Schema;', 1)[0]
    literals = [json.loads('"'+x+'"') for x in re.findall(r'TEXT\("((?:\\.|[^"\\])*)"\)', system_block)]
    order = 'companion_order仅允许目录中的高层指令：hold/follow/assist/routine；quantity=1,mode=directive,source=player。模型不选择敌人、坐标、路径、攻击时机或伤害。'
    prompt = literals[0]+intro+''.join(lines)+outro+literals[1]+order+''.join(literals[2:])
    with socket.socket() as reservation:
        reservation.bind(('127.0.0.1', 0))
        port = reservation.getsockname()[1]
    key = secrets.token_hex(24)
    base = f'http://127.0.0.1:{port}'

    def request(path, body=None):
        payload = None if body is None else json.dumps(body, ensure_ascii=False).encode('utf-8')
        req = urllib.request.Request(base+path, payload, {'Authorization': 'Bearer '+key, 'Content-Type': 'application/json'})
        with urllib.request.urlopen(req, timeout=10) as response:
            return json.load(response)

    command = [str(args.bundle/'bin'/args.backend/'llama-server.exe'), '-m',
               str(args.bundle/'models/Qwen3.5-4B-Q4_K_M.gguf'), '--host', '127.0.0.1',
               '--port', str(port), '--api-key', key, '--alias', 'hearthward-qwen-local',
               '-c', '4096', '-np', '1', '-t', '1', '-tb', '1', '-ngl',
               '0' if args.backend == 'cpu' else '16', '--reasoning', 'off', '--jinja',
               '--no-webui', '--no-cache-prompt']
    started = time.monotonic()
    process = subprocess.Popen(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    result = {'backend': args.backend, 'method': 'Actual locked llama-server /apply-template + /tokenize on System reconstructed from current source/data registry (24 capabilities, Describe and CompanionOrderPrompt); all world context omitted. No generation. Source reconstruction must be checked against the actual UE input-token result; this is not formal runtime acceptance.',
              'capability_count': len(rows), 'item_count': len(items), 'transport_item_count': len(stored), 'generation_calls': 0}
    try:
        deadline = time.monotonic()+120
        while True:
            if process.poll() is not None:
                raise RuntimeError('owned server exited with code '+str(process.returncode))
            try:
                request('/v1/models')
                break
            except Exception:
                if time.monotonic() >= deadline:
                    raise TimeoutError('owned server not ready within 120 seconds')
                time.sleep(.5)
        result['ready_seconds'] = round(time.monotonic()-started, 3)
        body = {'messages': [{'role': 'system', 'content': prompt}, {'role': 'user', 'content': '请新采四份木材并带回仓库'}],
                'chat_template_kwargs': {'enable_thinking': False}}
        applied = request('/apply-template', body)
        templated = applied['prompt']
        tokenized = request('/tokenize', {'content': templated, 'add_special': True, 'parse_special': True})
        result['partial_input_tokens'] = len(tokenized['tokens'])
        result['partial_prompt_already_exceeds_3328'] = result['partial_input_tokens'] > 3328
    except Exception as error:
        result['error'] = type(error).__name__+': '+str(error)
    finally:
        process.terminate()
        try:
            process.wait(timeout=8)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=8)
        result['owned_server_stopped'] = process.poll() is not None
    out = Path(__file__).resolve().parent/'prompt-budget-probe.json'
    out.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False), flush=True)
    raise SystemExit(1 if 'error' in result else 0)


if __name__ == '__main__':
    main()
