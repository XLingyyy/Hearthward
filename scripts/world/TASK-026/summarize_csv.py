"""Summarize native UE CSV frame samples; retain all raw frames including hitches."""
import argparse
import csv
import json
from pathlib import Path
import numpy as np


def summarize(path):
    with path.open(encoding='utf-8-sig', newline='') as f:
        rows = list(csv.reader(f))
    header = rows[0]
    index = header.index('FrameTime')
    data = []
    for row in rows[1:]:
        try:
            float(row[index])
        except (ValueError, IndexError):
            continue
        data.append(row)
    metadata = {}
    for row in rows:
        for i, key in enumerate(row[:-1]):
            if key in ['[engineversion]', '[systemresolution.resx]', '[systemresolution.resy]', '[rhi]', '[featurelevel]', '[gpu]']:
                metadata[key.strip('[]')] = row[i+1]
    series = {}
    for name in ['FrameTime', 'GameThreadTime', 'RenderThreadTime', 'GPUTime', 'MemoryFreeMB', 'GPUMem/LocalUsedMB']:
        values = np.array([float(r[header.index(name)]) for r in data])
        series[name] = {'mean':float(values.mean()), 'p95':float(np.percentile(values,95)),
                        'p99':float(np.percentile(values,99)), 'max':float(values.max())}
    ft = np.array([float(r[index]) for r in data])
    return {'raw':path.name, 'metadata':metadata, 'frames':len(data), 'seconds':float(ft.sum()/1000),
            'average_fps':float(1000/ft.mean()), 'hitches_at_least_1s':int((ft>=1000).sum()),
            'series':series, 'scope':'R0 stationary cost baseline; not R6 final acceptance'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    reports = [summarize(p) for p in sorted(args.directory.glob('*.csv'))]
    (args.directory/'summary.json').write_text(json.dumps(reports,indent=2),encoding='utf-8')
    print(json.dumps([{k:r[k] for k in ['raw','frames','seconds','average_fps','hitches_at_least_1s']} for r in reports],indent=2))
