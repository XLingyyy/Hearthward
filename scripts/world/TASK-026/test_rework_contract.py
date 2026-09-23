"""Regression tests for real overwrite/stale-plan failure modes in R1."""
import copy
import unittest
from rework_contract import make_plan, validate_plan, instances_equivalent


class ReworkPlanTests(unittest.TestCase):
    def setUp(self):
        self.instance = {'id': 'tree:0', 'transform': [[0, 0, 0], [0, 0, 0, 1], [1, 1, 1]]}
        self.batch = {'id': 'batch', 'package': '/Game/__ExternalActors__/Hearthward/World/Natural/Rebuild/a',
                      'ownership': 'Generated', 'cell': [-4, -3], 'kind': 'tree', 'instances': [self.instance]}
        self.authored = dict(self.batch, id='rock-anchor', ownership='Authored')
        self.inventory = {'actors': [self.batch, self.authored]}
        self.sources = {'height': 'unchanged', 'scatter': 'v2'}

    def test_plan_is_read_only_and_deterministic(self):
        before = copy.deepcopy(self.inventory)
        a = make_plan(self.inventory, {'batch': []}, self.sources, ['rock-anchor'])
        b = make_plan(self.inventory, {'batch': []}, self.sources, ['rock-anchor'])
        self.assertEqual(a, b)
        self.assertEqual(before, self.inventory)
        self.assertTrue(validate_plan(a, self.inventory, self.sources))
        self.assertEqual(a['update_packages'], [self.batch['package']])

    def test_authored_cannot_be_targeted(self):
        with self.assertRaisesRegex(ValueError, 'Protected'):
            make_plan(self.inventory, {'rock-anchor': []}, self.sources, ['rock-anchor'])

    def test_missing_protected_object_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Missing protected'):
            make_plan(self.inventory, {'batch': []}, self.sources, ['missing'])

    def test_stale_inputs_and_batches_rejected(self):
        p = make_plan(self.inventory, {'batch': []}, self.sources, ['rock-anchor'])
        with self.assertRaisesRegex(ValueError, 'Inputs changed'):
            validate_plan(p, self.inventory, {'height': 'changed'})
        self.batch['instances'] = []
        with self.assertRaisesRegex(ValueError, 'Batch changed'):
            validate_plan(p, self.inventory, self.sources)

    def test_duplicate_instances_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Duplicate'):
            make_plan(self.inventory, {'batch': [self.instance, self.instance]}, self.sources, [])

    def test_tampered_plan_rejected(self):
        p = make_plan(self.inventory, {'batch': []}, self.sources, ['rock-anchor'])
        p['operations'][0]['instances'] = [self.instance]
        with self.assertRaisesRegex(ValueError, 'Plan content changed'):
            validate_plan(p, self.inventory, self.sources)

    def test_native_quaternion_roundoff_is_not_a_new_edit(self):
        other = copy.deepcopy(self.instance)
        other['transform'][1][3] -= 3e-8
        self.assertTrue(instances_equivalent([self.instance], [other]))
        other['transform'][0][0] += 0.01
        self.assertFalse(instances_equivalent([self.instance], [other]))

    def test_native_quaternion_sign_flip_is_same_rotation(self):
        other = copy.deepcopy(self.instance)
        other['transform'][1] = [0, 0, 0, -1]
        self.assertTrue(instances_equivalent([self.instance], [other]))
        other['transform'][1] = [0, 0, -0.01, -0.99995]
        self.assertFalse(instances_equivalent([self.instance], [other]))

    def test_native_scale_roundoff_has_bounded_tolerance(self):
        other = copy.deepcopy(self.instance)
        other['transform'][2][0] += 3e-6
        self.assertTrue(instances_equivalent([self.instance], [other]))
        other['transform'][2][0] += .001
        self.assertFalse(instances_equivalent([self.instance], [other]))


if __name__ == '__main__':
    unittest.main()
