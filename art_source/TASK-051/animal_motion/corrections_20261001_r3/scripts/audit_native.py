"""Recheck the entire library because the paired skeletal skin changed."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import ROOT, REV, jobs, save
from audit_refinement import audit

if __name__=='__main__':
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    phase='candidate' if '--candidate' in args else 'final'
    for j in jobs(args):
        j=dict(j)
        if phase=='candidate':j['output']=str(REV/'candidate'/j['slug'])
        result=audit(j,'current')
        save(REV/phase/f'{j["slug"]}_audit.json',result)
