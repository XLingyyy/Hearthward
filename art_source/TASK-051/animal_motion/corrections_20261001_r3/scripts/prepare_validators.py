from common import ROOT,REV,R2

for name in ('audit_refinement.py','validate_revision_fbx.py','validate_revision_ue.py'):
    text=(R2/'scripts'/name).read_text(encoding='utf-8')
    text=text.replace('corrections_20261001_r2','corrections_20261001_r3').replace('Corrected20261001R2','Corrected20261001R3')
    (REV/'scripts'/name).write_text(text,encoding='utf-8')
print('Prepared current-file FBX and UE validators')
