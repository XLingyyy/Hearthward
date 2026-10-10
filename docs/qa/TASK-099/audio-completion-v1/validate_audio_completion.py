#!/usr/bin/env python3
"""Asset/configuration checks only. Does not run or emulate Unreal Engine."""
from pathlib import Path
import array
import hashlib
import json
import math
import subprocess
import sys
import unittest
import wave

ROOT = Path(__file__).resolve().parents[4]
BASE = '1bdc01b642dcf6e322ddca6cd4d7ad1a3fcc6351'
EXPECTED = {
    'movement.landed': ('movement-land.wav', 'effects', .78),
    'movement.swim.enter': ('movement-water-enter.wav', 'effects', 1.08),
    'movement.swim.exit': ('movement-water-exit.wav', 'effects', .88),
    'environment.fire.active': ('environment-fire-loop.wav', 'environment', 16),
    'environment.wind.lookouts': ('environment-wind-loop.wav', 'environment', 16),
}

class AudioCompletionAssets(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.config = json.loads((ROOT/'Resources/Data/experience.json').read_text(encoding='utf-8'))
        cls.events = {row['event_id']: row for row in cls.config['sound_events']}

    def test_five_distinct_bound_files(self):
        self.assertEqual(len(self.config['sound_events']), len(self.events))
        for event, (filename, channel, duration) in EXPECTED.items():
            row = self.events[event]
            self.assertEqual(row['file'], 'TASK-099/'+filename)
            self.assertEqual(row['channel'], channel)
        self.assertEqual(len({self.events[x]['file'] for x in EXPECTED}), 5)

    def test_pcm_format_headroom_and_endpoints(self):
        for event, (filename, channel, duration) in EXPECTED.items():
            with self.subTest(event=event):
                with wave.open(str(ROOT/'Resources/Audio/TASK-099'/filename), 'rb') as sound:
                    self.assertEqual((sound.getnchannels(), sound.getsampwidth(), sound.getframerate(), sound.getcomptype()), (1,2,48000,'NONE'))
                    self.assertAlmostEqual(sound.getnframes()/sound.getframerate(), duration, places=5)
                    data=array.array('h', sound.readframes(sound.getnframes()))
                    if sys.byteorder != 'little': data.byteswap()
                self.assertGreater(max(data)-min(data), 2000)
                self.assertLess(max(abs(x) for x in data), 30000)
                self.assertLess(abs(sum(data)/len(data)), 100)
                if channel == 'environment':
                    self.assertLess(abs(data[0]-data[-1]), 100, 'loop seam must not contain a large sample jump')
                else:
                    self.assertLessEqual(abs(data[0]), 2)
                    self.assertLessEqual(abs(data[-1]), 2)

    def test_local_wind_rule_and_continuous_edge_gain(self):
        source=self.events['environment.wind.lookouts']['source']
        self.assertTrue(source['enabled'])
        self.assertEqual(source['kind'], 'landmark_wind')
        self.assertEqual(set(source['tags']), {'CampaignNode:route_ridge','CampaignNode:route_watch'})
        self.assertEqual(source['asset'], '/Game/Hearthward/Assets/TASK-097/Route/SM_RouteLookout.SM_RouteLookout')
        inner,outer=source['inner_radius_cm'],source['radius_cm']
        self.assertTrue(0 < inner < outer <= 20000)
        self.assertTrue(0 < source['gain'] < 1 and 0 < source['roof_trace_cm'] <= 10000)
        def gain(distance):
            edge=max(0,min(1,(outer-distance)/(outer-inner)))
            return edge*edge*(3-2*edge)
        self.assertEqual(gain(inner),1)
        self.assertEqual(gain(outer),0)
        values=[gain(inner+(outer-inner)*i/100) for i in range(101)]
        self.assertTrue(all(a>=b for a,b in zip(values,values[1:])))
        self.assertLess(gain(outer-1),1e-6)

    def test_fire_is_bound_to_actual_flame_not_smoke(self):
        source=self.events['environment.fire.active']['source']
        self.assertTrue(source['enabled'])
        self.assertEqual(source['kind'],'active_flame')
        self.assertEqual(source['asset'],'/Game/Hearthward/Assets/TASK-096/Fire/NS_HearthFire.NS_HearthFire')
        self.assertEqual(set(source['tags']),{'Hearthward.FacilityFire','HearthwardRaidVFX'})
        self.assertTrue(1<=source['max_sources']<=4)
        self.assertTrue(0<source['radius_cm']<=5000)
        self.assertTrue(0<source['gain']<1)

    def test_existing_voice_lake_and_all_other_fields_preserved(self):
        try:
            raw=subprocess.check_output(['git','show',BASE+':Resources/Data/experience.json'],cwd=ROOT,stderr=subprocess.DEVNULL)
        except (FileNotFoundError,subprocess.CalledProcessError):
            self.skipTest('Baseline Git object unavailable; compare against the stated base separately')
        baseline=json.loads(raw)
        current=dict(self.config)
        current['sound_events']=[row for row in current['sound_events'] if row['event_id'] not in EXPECTED]
        # TASK-104 intentionally replaces the neutral footstep and adds four surfaces.
        # Validate those exact additions before comparing all unrelated fields.
        footsteps={row['event_id']:row for row in current['sound_events'] if row['event_id'].startswith('movement.footstep')}
        self.assertEqual(set(footsteps), {'movement.footstep', *(f'movement.footstep.{s}' for s in ('grass','dirt','stone','wood'))})
        for event,row in footsteps.items():
            surface='unknown' if event=='movement.footstep' else event.rsplit('.',1)[1]
            variants=[f'TASK-104/footstep-{surface}-{i:02d}.wav' for i in range(1,4)]
            self.assertEqual(row['file'], variants[0])
            self.assertEqual(row['variants'], variants)
            self.assertEqual(row['channel'], 'effects')
        current['sound_events']=[row for row in current['sound_events'] if not row['event_id'].startswith('movement.footstep')]
        baseline['sound_events']=[row for row in baseline['sound_events'] if not row['event_id'].startswith('movement.footstep')]
        self.assertEqual(current,baseline)

    def test_runtime_license_is_nonempty(self):
        notice=(ROOT/'Resources/Audio/TASK-099/License-Audio-Completion-v1.txt').read_text(encoding='utf-8')
        for filename,_,_ in EXPECTED.values(): self.assertIn(filename,notice)
        self.assertIn('CC0',notice)
        self.assertIn('procedural',notice)

if __name__=='__main__':
    unittest.main(verbosity=2)
