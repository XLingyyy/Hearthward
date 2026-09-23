"""Refresh the multi-scale rock surface without changing height or collision."""
from pathlib import Path
import sys,unreal
sys.path.insert(0,str(Path(__file__).parent))
from rebuild_terrain import ground_material
ground_material()
unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'grass.FlushCache')
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
