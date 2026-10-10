"""Headless asset/data checks. Native UE tests and subjective listening are separate gates.

CI can keep LFS pointers: deterministic synthesis is analyzed as PCM and its exact
bytes must match the pointer OID/size. This never claims the pointer itself plays.
"""
import array
from functools import lru_cache
import hashlib
import importlib.util
import io
import json
import math
from pathlib import Path
import struct
import sys
import unittest
import wave

ROOT = Path(__file__).resolve().parents[2]
SURFACES = ('unknown', 'grass', 'dirt', 'stone', 'wood')


def module(name, relative):
    spec = importlib.util.spec_from_file_location(name, ROOT / relative)
    result = importlib.util.module_from_spec(spec)
    sys.modules[name] = result
    spec.loader.exec_module(result)
    return result


@lru_cache(maxsize=1)
def generated():
    return module('surface_audio104', 'scripts/audio/generate_surface_footsteps.py').make_outputs()


def assert_asset(test, path, expected):
    actual = path.read_bytes()
    if actual.startswith(b'version https://git-lfs.github.com/spec/v1'):
        fields = dict(line.split(' ', 1) for line in actual.decode('ascii').splitlines())
        test.assertEqual(fields['oid'], 'sha256:'+hashlib.sha256(expected).hexdigest(), str(path))
        test.assertEqual(int(fields['size']), len(expected), str(path))
    else:
        test.assertEqual(hashlib.sha256(actual).hexdigest(), hashlib.sha256(expected).hexdigest(), str(path))
        test.assertEqual(len(actual), len(expected), str(path))


class SurfaceFootstepAssetsTests(unittest.TestCase):
    def test_15_runtime_assets_match_deterministic_sources_or_lfs_oids(self):
        outputs, review = generated()
        names = {f'footstep-{surface}-{n:02}.wav' for surface in SURFACES for n in range(1, 4)}
        self.assertEqual({name for name in outputs if name.endswith('.wav')}, names)
        for name in names:
            assert_asset(self, ROOT / 'Resources/Audio/TASK-104' / name, outputs[name])
        assert_asset(self, ROOT / 'art_source/TASK-104/footstep-review.wav', review)
        self.assertEqual((ROOT / 'Resources/Audio/TASK-104/PROVENANCE.json').read_bytes(), outputs['PROVENANCE.json'])

    def test_synthesized_pcm_is_non_silent_distinct_and_clean(self):
        outputs, _ = generated()
        hashes = set()
        for name, data in outputs.items():
            if not name.endswith('.wav'):
                continue
            hashes.add(hashlib.sha256(data).hexdigest())
            with wave.open(io.BytesIO(data), 'rb') as wav:
                self.assertEqual((wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getcomptype()), (1, 2, 44100, 'NONE'))
                self.assertGreater(wav.getnframes(), 4410)
                self.assertLess(wav.getnframes(), 44100)
                samples = array.array('h', wav.readframes(wav.getnframes()))
                if sys.byteorder != 'little':
                    samples.byteswap()
            peak = max(abs(x) for x in samples)
            rms = math.sqrt(sum(x*x for x in samples)/len(samples))
            self.assertGreater(peak, 1000, name)
            self.assertLess(peak, 32767, name)
            self.assertGreater(rms, 100, name)
            self.assertLess(abs(sum(samples)/len(samples)), 20, name)
            self.assertLessEqual(max(abs(x) for x in samples[-132:]), 3, name)
        self.assertEqual(len(hashes), 15, 'Variants and surfaces must not be renamed identical files')

    def test_each_surface_event_has_three_real_effects_variants(self):
        data = json.loads((ROOT / 'Resources/Data/experience.json').read_text(encoding='utf-8'))
        rows = {r['event_id']: r for r in data['sound_events']}
        for surface in SURFACES:
            event = 'movement.footstep' + ('.'+surface if surface != 'unknown' else '')
            row = rows[event]
            expected = [f'TASK-104/footstep-{surface}-{i:02}.wav' for i in range(1, 4)]
            self.assertEqual(row['variants'], expected)
            self.assertEqual(row['file'], expected[0])
            self.assertEqual(row['channel'], 'effects')
            self.assertIn('ORIGINAL_PROCEDURAL', row['source_status'])
        self.assertNotIn('footstep-review.wav', json.dumps(data))


class SurfaceFootstepTerrainTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bake = module('surface_terrain104', 'scripts/audio/prepare_footstep_terrain.py')
        cls.data = (ROOT / 'Resources/Audio/TASK-104/terrain-surfaces.rle').read_bytes()
        cls.manifest = json.loads((ROOT / 'art_source/TASK-104/terrain-surfaces.json').read_text(encoding='utf-8'))

    def test_baked_material_source_fingerprints(self):
        self.assertEqual(hashlib.sha256(self.data).hexdigest(), self.manifest['output_sha256'])
        for source in self.manifest['sources']:
            data = (ROOT / source['path']).read_bytes()
            if data.startswith(b'version https://git-lfs.github.com/spec/v1'):
                self.assertIn(('oid sha256:'+source['sha256']).encode(), data)
            else:
                self.assertEqual(hashlib.sha256(data).hexdigest(), source['sha256'])

    def test_grid_covers_real_grass_dirt_and_rock_samples(self):
        side, cells = self.bake.decode(self.data)
        self.assertEqual(side, 2017)
        self.assertEqual(len(cells), side*side)
        self.assertEqual(set(cells), {1, 2, 3})
        for ident, name in enumerate(('grass', 'dirt', 'stone'), 1):
            self.assertEqual(cells.count(ident), self.manifest['cell_counts'][name])
        for x, y, expected in [(-98000, -75000, 1), (-110000, -70000, 2), (60000, 0, 2), (-30000, 140000, 3)]:
            col = math.floor((x+201600)/403200*side)
            row = math.floor((y+201600)/403200*side)
            self.assertEqual(cells[row*side+col], expected)

    def test_corrupt_missing_and_oversized_runs_are_rejected(self):
        header = b'HWS1'+struct.pack('<I', 2017)
        for bad in [b'', b'HWS2'+self.data[4:], self.data[:-1], header+struct.pack('<BH', 1, 0),
                    header+struct.pack('<BH', 4, 3), b'HWS1'+struct.pack('<I', 9999),
                    self.data+struct.pack('<BH', 1, 1), header+struct.pack('<BH', 1, 1)]:
            with self.assertRaises(ValueError):
                self.bake.decode(bad)


class SurfaceFootstepWiringTests(unittest.TestCase):
    def test_actual_ground_receipt_drives_surface_and_keeps_guards(self):
        notify = (ROOT / 'Source/Hearthward/Experience/HearthwardFootContactNotify.cpp').read_text(encoding='utf-8')
        for token in ('Query.bReturnPhysicalMaterial=true', 'Query.bReturnFaceIndex=true', 'Receipt.Surface=HearthwardFootstepSurface::Resolve(Hit)',
                      'IsMovingOnGround()', 'IsWalkableFloor()', 'Movement->IsWalkable(Hit)', 'World->IsPaused()', 'Survival->Alive()'):
            self.assertIn(token, notify)
        presentation = (ROOT / 'Source/Hearthward/Experience/HearthwardPresentationComponent.cpp').read_text(encoding='utf-8')
        for token in ('EventFor(Receipt.Surface)', 'SelectVariant(Count,Cue->LastVariant', 'Receipt.Frame!=GFrameCounter', 'Receipt.Epoch!=Epoch',
                      'ObservedFootContacts.Contains(Receipt.SuccessId)', 'Pair.Value.LastVariant=INDEX_NONE',
                      'Event!=TEXT("movement.footstep") && PlaySoundEvent(TEXT("movement.footstep")'):
            self.assertIn(token, presentation)

    def test_runtime_dependencies_stage_assets_and_native_tests_exist(self):
        build = (ROOT / 'Source/Hearthward/Hearthward.Build.cs').read_text(encoding='utf-8')
        self.assertIn('"PhysicsCore"', build)
        self.assertIn('Directory.GetFiles(Resources, "*", SearchOption.AllDirectories)', build)
        tests = (ROOT / 'Source/Hearthward/Tests/SurfaceFootstepTests.cpp').read_text(encoding='utf-8')
        for suffix in ('SurfacePolicy', 'NoImmediateRepeat', 'AuthoredTerrain', 'ActualHitAndPhysicalPriority', 'PlaybackFallbackAndRestore'):
            self.assertIn('Hearthward.Iteration.Task104.Footstep.'+suffix, tests)


if __name__ == '__main__':
    unittest.main()
