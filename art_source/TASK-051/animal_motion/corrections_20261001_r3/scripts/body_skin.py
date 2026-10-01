"""Conservative correction of hare torso weights blended from the old skin."""
import numpy as np
from refinement_helpers_r1 import repair_torso

def matrix(mesh):
    result=np.zeros((len(mesh.data.vertices),len(mesh.vertex_groups)),np.float32)
    for v in mesh.data.vertices:
        for g in v.groups:result[v.index,g.group]=g.weight
    return result

def repair_hare_torso(arm,meshes,rig,gain=.3):
    original={o.name:matrix(o) for o in meshes}
    repaired=repair_torso(arm,meshes,rig,'hare');changed=0
    for mesh in meshes:
        old=original[mesh.name];target=matrix(mesh)
        if target.shape[1]>old.shape[1]:old=np.pad(old,((0,0),(0,target.shape[1]-old.shape[1])))
        mixed=old+(target-old)*gain
        for v,w in zip(mesh.data.vertices,mixed):
            keep=np.argsort(w)[-8:];total=float(w[keep].sum())
            for g in list(v.groups):mesh.vertex_groups[g.group].remove([v.index])
            for i in keep:
                if w[i]>1e-7:mesh.vertex_groups[int(i)].add([v.index],float(w[i])/total,'REPLACE')
            if np.max(abs(w-old[v.index]))>1e-6:changed+=1
    return {'changed_vertices':changed,'full_repair_vertices':repaired,'gain':gain,'method':'30 percent measured torso reassignment; normalized at most eight influences'}
