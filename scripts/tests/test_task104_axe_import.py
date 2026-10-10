"""Stdlib simulation of importer guards, NOT Unreal execution or asset/render QA.

The real importer runs with runpy against a deliberately tiny fake Unreal API and
a temporary project. These tests prove Python control flow and preservation
checks; they cannot prove UE method availability, rebuild behavior, or persistence.
"""
import copy
import hashlib
import json
import math
from pathlib import Path
import runpy
import tempfile
import types
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'scripts/equipment/task104_apply_axe_grip.py'
ASSET = 'Content/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe.uasset'
MESH_PATH = '/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe'
APPLY = '-Task104AxeApply -Task104AxeLockId=owned-104'


class Vector:
    def __init__(self, x=0, y=0, z=0):
        self.x, self.y, self.z = float(x), float(y), float(z)

    def __sub__(self, other):
        return Vector(self.x-other.x, self.y-other.y, self.z-other.z)

    def length(self):
        return math.sqrt(self.x*self.x+self.y*self.y+self.z*self.z)


class Element:
    def __init__(self, id_value=0):
        self.id_value = id_value


class Properties:
    def __init__(self, **properties):
        self.properties = properties

    def get_editor_property(self, name):
        # UE returns fresh struct wrappers; never let stable Python repr mask the
        # historical bug of serializing socket rotations with str(wrapper).
        return copy.deepcopy(self.properties[name])

    def set_editor_property(self, name, value):
        self.properties[name] = value


class Rotation:
    def __init__(self, pitch=0, yaw=0, roll=0):
        self.pitch, self.yaw, self.roll = pitch, yaw, roll


class Material:
    def __init__(self, path='/Game/Original.Material'):
        self.path = path

    def get_path_name(self):
        return self.path


class Description:
    def __init__(self):
        self.positions = [Vector(1, 0, 0), Vector(0, 1, 0), Vector(0, 0, 9)]
        self.uvs = [[Vector(0, 0)], [Vector(1, 0)], [Vector(0, 1)]]
        self.instances = [0, 1, 2]
        self.triangles = [[0, 1, 2]]
        self.groups = [0]
        self.mutations = 0

    def get_vertex_count(self): return len(self.positions)
    def get_vertex_instance_count(self): return len(self.instances)
    def get_triangle_count(self): return len(self.triangles)
    def is_vertex_valid(self, element): return 0 <= element.id_value < len(self.positions)
    def is_vertex_instance_valid(self, element): return 0 <= element.id_value < len(self.instances)
    def is_triangle_valid(self, element): return 0 <= element.id_value < len(self.triangles)
    def get_vertex_position(self, element): return copy.deepcopy(self.positions[element.id_value])
    def get_vertex_instance_vertex(self, element): return Element(self.instances[element.id_value])
    def get_vertex_instance_uv(self, element, channel): return self.uvs[element.id_value][channel]
    def get_triangle_vertex_instances(self, element): return [Element(i) for i in self.triangles[element.id_value]]
    def get_triangle_polygon_group(self, element): return Element(self.groups[element.id_value])

    def set_vertex_position(self, element, position):
        self.mutations += 1
        self.positions[element.id_value] = copy.deepcopy(position)


class Mesh:
    def __init__(self):
        self.description = Description()
        self.materials = [Material()]
        self.sockets = {name: Properties(relative_location=Vector(i, 0, 0),
                        relative_rotation=Rotation(1, 2, 3), relative_scale=Vector(1, 1, 1))
                        for i, name in enumerate(('Grip', 'BladeBase', 'BladeTip'))}
        self.builds = 0
        self.build_hook = None

    def find_socket(self, name): return self.sockets.get(name)
    def get_material(self, index): return self.materials[index]
    def get_editor_property(self, name):
        assert name == 'static_materials'
        return self.materials
    def get_static_mesh_description(self, lod):
        assert lod == 0
        return self.description
    def get_path_name(self): return MESH_PATH + '.SM_stone_bone_axe'
    def get_num_triangles(self, lod): return self.description.get_triangle_count()

    def build_from_static_mesh_descriptions(self, descriptions, collision, fast):
        assert descriptions == [self.description] and not collision and not fast
        self.builds += 1
        if self.build_hook:
            self.build_hook(self)


class ImportGuardSimulationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.project = Path(self.temp.name)
        self.mesh = Mesh()
        self.saved = []
        self.dirty_content, self.dirty_maps, self.pie = [], [], False
        self.command = APPLY
        self.locks = {'ours': [{'id': 'owned-104', 'path': ASSET}], 'theirs': []}
        self.asset_file = self.project / ASSET
        self.asset_file.parent.mkdir(parents=True)
        self.asset_file.write_bytes(b'fake original uasset; not an Unreal package')
        self.manifest = {'mesh_path': MESH_PATH, 'source_vertex_count': 3,
                         'source_uasset_sha256': hashlib.sha256(self.asset_file.read_bytes()).hexdigest(),
                         'vertices_id_original_xyz_new_xyz': [[0, 1, 0, 0, .25, 0, 0]]}
        self.write_json('art_source/TASK-104/grip/handle_fit_delta.json', self.manifest)
        self.write_json('docs/qa/TASK-055/stone-axe-measured-endpoints.json',
                        {'candidates': {name: [i, 0, 0] for i, name in enumerate(self.mesh.sockets)}})
        self.receipt = self.project / 'Saved/Task104/axe-grip-accepted.json'
        self.unreal = types.ModuleType('unreal')
        u = self.unreal
        u.Vector, u.VertexID, u.VertexInstanceID, u.TriangleID = Vector, Element, Element, Element
        u.StaticMesh = Mesh
        u.Paths = types.SimpleNamespace(project_dir=lambda: str(self.project),
                                       project_saved_dir=lambda: str(self.project/'Saved'))
        u.SystemLibrary = types.SimpleNamespace(get_command_line=lambda: self.command)
        u.EditorLoadingAndSavingUtils = types.SimpleNamespace(
            get_dirty_content_packages=lambda: self.dirty_content,
            get_dirty_map_packages=lambda: self.dirty_maps)
        u.EditorAssetLibrary = types.SimpleNamespace(save_loaded_asset=self.save_asset)
        u.AutomationLibrary = types.SimpleNamespace(finish_loading_before_screenshot=lambda: None)
        u.StaticMeshEditorSubsystem = type('StaticMeshEditorSubsystem', (), {})
        u.LevelEditorSubsystem = type('LevelEditorSubsystem', (), {})
        u.UnrealEditorSubsystem = type('UnrealEditorSubsystem', (), {})
        editor = types.SimpleNamespace(get_simple_collision_count=lambda mesh: 0,
            get_convex_collision_count=lambda mesh: 0, get_num_uv_channels=lambda mesh, lod: 1,
            get_lod_build_settings=lambda mesh, lod: Properties(),
            set_lod_build_settings=lambda mesh, lod, options: None)
        level = types.SimpleNamespace(is_in_play_in_editor=lambda: self.pie,
                                      get_game_world=lambda: object() if self.pie else None)
        u.get_editor_subsystem = lambda cls: level if cls in (u.LevelEditorSubsystem, u.UnrealEditorSubsystem) else editor
        u.load_asset = lambda path: self.mesh if path == MESH_PATH else None
        u.log = lambda message: None

    def write_json(self, name, data):
        path = self.project / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(data), encoding='utf-8')

    def save_asset(self, mesh, only_if_is_dirty=False):
        self.assertIs(mesh, self.mesh)
        self.saved.append(mesh.get_path_name())
        return True

    def run_import(self, command=APPLY):
        self.command = command
        with patch.dict('sys.modules', {'unreal': self.unreal}), \
             patch('subprocess.check_output', return_value=json.dumps(self.locks)):
            result = runpy.run_path(str(SCRIPT))
        return result['report']

    def assert_rejected_before_mutation(self, report, message):
        self.assertFalse(report['ok'], report)
        self.assertIn('AssertionError', report['error'])
        self.assertNotIn('AttributeError', report['error'])
        self.assertIn(message, report['error'])
        self.assertEqual((self.mesh.description.mutations, self.mesh.builds, self.saved), (0, 0, []))
        self.assertFalse(self.receipt.exists())

    def test_default_probe_never_mutates_or_saves_uncorrected_asset(self):
        self.assert_rejected_before_mutation(self.run_import(''), 'Corrected geometry is not present')

    def test_missing_lock_rejects_before_mutation(self):
        self.assert_rejected_before_mutation(self.run_import('-Task104AxeApply'), 'LFS lock ID')

    def test_foreign_or_wrong_path_lock_rejects_before_mutation(self):
        for locks in ({'ours': [], 'theirs': self.locks['ours']},
                      {'ours': [{'id': 'owned-104', 'path': 'Content/Wrong.uasset'}]}):
            with self.subTest(locks=locks):
                self.locks = locks
                self.assert_rejected_before_mutation(self.run_import(), 'lock is not owned')

    def test_wrong_lock_id_rejects_before_mutation(self):
        self.assert_rejected_before_mutation(
            self.run_import('-Task104AxeApply -Task104AxeLockId=wrong'), 'lock is not owned')

    def test_source_hash_mismatch_rejects_before_mutation(self):
        self.asset_file.write_bytes(b'changed upstream asset')
        self.assert_rejected_before_mutation(self.run_import(), 'source uasset hash changed')

    def test_dirty_content_rejects_before_mutation(self):
        self.dirty_content = ['UnsavedAsset']
        self.assert_rejected_before_mutation(self.run_import(), 'unsaved')

    def test_dirty_maps_reject_before_mutation(self):
        self.dirty_maps = ['UnsavedMap']
        self.assert_rejected_before_mutation(self.run_import(), 'unsaved')

    def test_active_pie_rejects_before_mutation(self):
        self.pie = True
        report = self.run_import()
        self.assert_rejected_before_mutation(report, 'PIE')

    def test_valid_delta_preserves_every_unselected_property_and_saves_only_axe(self):
        report = self.run_import()
        self.assertTrue(report['ok'], report)
        self.assertEqual((self.mesh.description.mutations, self.mesh.builds), (1, 1))
        self.assertEqual(self.saved, [self.mesh.get_path_name()])
        self.assertEqual(report['saved_assets'], self.saved)
        self.assertEqual(self.mesh.description.positions[0].x, .25)
        self.assertEqual(self.mesh.description.positions[2].z, 9)
        self.assertEqual(report['topology_uv_sha256_before'], report['topology_uv_sha256_after'])
        self.assertEqual(report['material_paths'], ['/Game/Original.Material'])
        self.assertEqual(report['sockets']['Grip']['rotation'], [1, 2, 3])
        self.assertEqual(report['collision_counts'], [0, 0])
        self.assertTrue(self.receipt.exists())

    def test_repeated_apply_and_probe_keep_original_preservation_receipt(self):
        self.assertTrue(self.run_import()['ok'])
        baseline = self.receipt.read_bytes()
        for command in (APPLY, ''):
            report = self.run_import(command)
            self.assertTrue(report['ok'], report)
            self.assertTrue(report['already_applied'])
            self.assertEqual(report['saved_assets'], [])
            self.assertEqual(self.receipt.read_bytes(), baseline)
        self.assertEqual((self.mesh.description.mutations, self.mesh.builds, len(self.saved)), (1, 1, 1))

    def test_changed_head_rejected_by_repeated_apply_and_probe_without_rebaseline(self):
        self.assertTrue(self.run_import()['ok'])
        baseline = self.receipt.read_bytes()
        self.mesh.description.positions[2].z = 12
        for command in (APPLY, ''):
            report = self.run_import(command)
            self.assertFalse(report['ok'], report)
            self.assertIn('geometry changed', report['error'])
            self.assertEqual(self.receipt.read_bytes(), baseline)
        self.assertEqual(len(self.saved), 1)

    def test_changed_material_rejected_by_repeated_apply_and_probe_without_rebaseline(self):
        self.assertTrue(self.run_import()['ok'])
        baseline = self.receipt.read_bytes()
        self.mesh.materials[0] = Material('/Game/Changed.Material')
        for command in (APPLY, ''):
            report = self.run_import(command)
            self.assertFalse(report['ok'], report)
            self.assertIn('asset properties changed', report['error'])
            self.assertEqual(self.receipt.read_bytes(), baseline)
        self.assertEqual(len(self.saved), 1)

    def test_corrected_geometry_without_receipt_cannot_be_rebaselined(self):
        self.mesh.description.positions[0].x = .25
        self.assert_rejected_before_mutation(self.run_import(), 'cannot be rebaselined')

    def test_rebuild_uv_corruption_is_rejected_before_asset_save(self):
        self.mesh.build_hook = lambda mesh: setattr(mesh.description.uvs[0][0], 'x', .5)
        report = self.run_import()
        self.assertFalse(report['ok'], report)
        self.assertIn('UV values', report['error'])
        self.assertEqual(self.saved, [])
        self.assertFalse(self.receipt.exists())

    def test_rebuild_socket_change_is_rejected_before_asset_save(self):
        self.mesh.build_hook = lambda mesh: setattr(mesh.sockets['Grip'].properties['relative_rotation'], 'yaw', 99)
        report = self.run_import()
        self.assertFalse(report['ok'], report)
        self.assertIn('Rebuild changed authored sockets', report['error'])
        self.assertEqual(self.saved, [])
        self.assertFalse(self.receipt.exists())

    def test_rebuild_added_material_slot_is_rejected_before_asset_save(self):
        self.mesh.build_hook = lambda mesh: mesh.materials.append(Material('/Game/Unexpected.Material'))
        report = self.run_import()
        self.assertFalse(report['ok'], report)
        self.assertIn('Material slot count changed', report['error'])
        self.assertEqual(self.saved, [])
        self.assertFalse(self.receipt.exists())


if __name__ == '__main__':
    unittest.main()
