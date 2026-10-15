"""Run with UE's native ProgrammaticToolset after StaticMeshTools.import_file."""
import json

def slots(mesh):
    return execute_tool('editor_toolset.toolsets.static_mesh.StaticMeshTools.get_material_slots',json.dumps({'mesh':mesh}))['returnValue']
def assign(mesh,slot,material):
    return execute_tool('editor_toolset.toolsets.static_mesh.StaticMeshTools.set_material',json.dumps({'mesh':mesh,'slot_name':slot,'material':{'refPath':material}}))['returnValue']
def bounds(mesh):
    return execute_tool('editor_toolset.toolsets.static_mesh.StaticMeshTools.get_bounds',json.dumps({'mesh':mesh}))['returnValue']
def save(paths):
    return execute_tool('editor_toolset.toolsets.asset.AssetTools.save_assets',json.dumps({'asset_paths':paths}))
def run():
    materials={
        'OldTimber':'/Game/Hearthward/Assets/TASK-096/Nearfield/M_OldTimber.M_OldTimber',
        'ForgedIron':'/Game/Hearthward/Assets/TASK-098/Workbench/M_PracticalIron.M_PracticalIron',
        'Hemp':'/Game/Hearthward/Assets/TASK-098/CampSet/M_CampRope.M_CampRope',
        'AgedBronze':'/Game/Hearthward/Assets/TASK-098/CampSet/M_CampIron.M_CampIron',
        'LinenRecord':'/Game/Hearthward/Assets/TASK-098/CampSet/M_CampLinen.M_CampLinen'}
    names=['Winch','Ladder','Beam','Latch','NoisePlate','Lamp','ToolsMemento','HerbRecord','WatchMemento']
    result=[];paths=[]
    for suffix in names:
        name='SM_Round2_'+suffix;path='/Game/Hearthward/Assets/TASK-118/'+name+'.'+name;mesh={'refPath':path}
        nameslots=slots(mesh)
        for slot in nameslots:
            if not assign(mesh,slot,materials[slot]):raise RuntimeError('Material assignment failed: '+slot)
        result.append({'name':name,'bounds_cm':bounds(mesh),'material_slots':nameslots});paths.append(path)
    save(paths)
    return {'meshes':result,'material_source':'Existing Hearthward approved asset library; original source geometry'}
