"""Offline geometry/scale regression. Does not build C++ or replace UE pose/render QA."""
import json
import math
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def rotate(q, v):
    x, y, z, w = q
    cross = lambda a, b: (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
    uv = cross((x, y, z), v)
    uuv = cross((x, y, z), uv)
    return tuple(v[i] + 2 * (w * uv[i] + uuv[i]) for i in range(3))


class AxeHandleGripTests(unittest.TestCase):
    def test_production_socket_is_the_measured_handle(self):
        measured = json.loads((ROOT / "docs/qa/TASK-055/stone-axe-measured-endpoints.json").read_text(encoding="utf-8"))
        probe = json.loads((ROOT / "docs/qa/TASK-055/stone-axe-sockets-probe-results.json").read_text(encoding="utf-8"))
        self.assertEqual(measured["candidates"]["Grip"], probe["actual"]["Grip"]["location_cm"])
        self.assertGreater(math.dist(measured["candidates"]["Grip"], (0, 0, 0)) * .7, 27)

    def test_absolute_scale_compensation_under_recorded_animation_poses(self):
        samples = json.loads((ROOT / "docs/qa/TASK-055/stone-axe-grip-candidate-results.json").read_text(encoding="utf-8"))["clips"]["current_attack"]["samples"]
        grip = json.loads((ROOT / "docs/qa/TASK-055/stone-axe-measured-endpoints.json").read_text(encoding="utf-8"))["candidates"]["Grip"]
        palm = (-.02, -.03, .01)  # synthetic target; not a visual fit claim
        # Arbitrary normalized rotation ensures this tests more than the old -90 roll.
        rotation = (.5, -.5, .5, .5)
        offset = rotate(rotation, tuple(v * .7 for v in grip))
        for sample in samples:
            hand = sample["actual_hand_socket_world"]
            for character_scale_ratio in (1, (160 / 97.863766) / (180 / 97.869893)):
                scale = [v * character_scale_ratio for v in hand["scale_xyz"]]
                relative = [palm[i] - offset[i] / scale[i] for i in range(3)]
                origin = rotate(hand["rotation_quat_xyzw"], [relative[i] * scale[i] for i in range(3)])
                grip_offset = rotate(hand["rotation_quat_xyzw"], offset)
                expected = rotate(hand["rotation_quat_xyzw"], [palm[i] * scale[i] for i in range(3)])
                self.assertLess(math.dist([origin[i] + grip_offset[i] for i in range(3)], expected), 1e-9)
                wrong = [palm[i] - offset[i] / (scale[i] / 100) for i in range(3)]
                self.assertGreater(math.dist([wrong[i]*scale[i]+offset[i] for i in range(3)], [palm[i]*scale[i] for i in range(3)]), 1000)

    def test_both_actors_use_shared_solver(self):
        hero = (ROOT / "Source/Hearthward/HearthwardCharacter.cpp").read_text(encoding="utf-8")
        brother = (ROOT / "Source/Hearthward/Companion/HearthwardCompanionFixture.cpp").read_text(encoding="utf-8")
        self.assertIn("HearthwardAxeGrip::Build(HeroMesh.Object", hero)
        self.assertIn("HearthwardAxeGrip::Build(BrotherMesh.Object", brother)
        self.assertIn("Axe?AxeGrip:WeaponGrip", brother)
        self.assertNotIn("HeldAxe->SetRelativeRotation(FRotator(0,0,-90))", hero)
        self.assertNotIn("Axe?FTransform(FRotator(0,0,-90)", brother)

    def test_cpp_profiles_match_verified_pose_data(self):
        import re
        profiles = json.loads((ROOT / "art_source/TASK-104/grip/hand_pose_profiles.json").read_text(encoding="utf-8"))["profiles"]
        source = (ROOT / "Source/Hearthward/Equipment/HearthwardAxeHandPose.cpp").read_text(encoding="utf-8")
        expected_names = {f"{finger}_{joint:02d}_r" for finger in ("index", "middle", "ring", "pinky", "thumb") for joint in (1, 2, 3)}
        names_line = source.split("static const TArray<FName> Names =", 1)[1].split(";", 1)[0]
        self.assertEqual(set(re.findall(r'TEXT\("([^"]+)"\)', names_line)), expected_names)
        self.assertNotIn("hand_r", names_line)
        for role, profile in profiles.items():
            block = source.split(f'Mesh->GetFName() == TEXT("SK_{role}")', 1)[1].split("return true;", 1)[0]
            anchor = [float(v) for v in re.search(r"PalmInHand = FVector\(([^)]+)\)", block).group(1).split(",")]
            self.assertLess(math.dist(anchor, profile["palm_hand_local"]), 1e-11)
            quats = {name: [float(v) for v in values.split(",")]
                     for values, name in re.findall(r"FQuat\(([^)]+)\), // (\w+)", block)}
            self.assertEqual(set(quats), expected_names)
            for name, actual in quats.items():
                self.assertLess(math.dist(actual, profile["target_fingers"][name]), 1e-11)
                self.assertAlmostEqual(sum(v*v for v in actual), 1.0, places=9)
        for role in ("Hero", "Brother"):
            graph = (ROOT / f"Source/Hearthward/Animation/Hearthward{role}AnimInstance.cpp").read_text(encoding="utf-8")
            self.assertIn("return &AxeFingers;", graph)
            self.assertIn("AxeFingers.Enabled = HearthwardAxeHandPose::ShouldApply", graph)

    def test_offline_full_rig_clearance_and_wrap_evidence(self):
        # Evidence-consistency check only; does not simulate UE or certify visual quality.
        report = json.loads((ROOT / "docs/qa/TASK-104/grip/full-rig-fit-summary.json").read_text(encoding="utf-8"))
        wrap = json.loads((ROOT / "docs/qa/TASK-104/grip/finger-wrap-metrics.json").read_text(encoding="utf-8"))
        profiles = json.loads((ROOT / "art_source/TASK-104/grip/hand_pose_profiles.json").read_text(encoding="utf-8"))["profiles"]
        self.assertEqual({r["role"] for r in report}, {"Hero", "Brother"})
        for row in report:
            self.assertGreater(row["min_axis_clearance_cm"], .9)
            self.assertEqual(row["triangles_inside_0_9cm_radius"], 0)
            self.assertEqual(row["palm_hand_local"], profiles[row["role"]]["palm_hand_local"])
        for row in wrap:
            self.assertGreater(row["tip_centres_angular_span_degrees"], 180)
            self.assertEqual({d["finger"] for d in row["digits"]}, {"index", "middle", "ring", "pinky", "thumb"})

    def test_corrected_source_preserves_head_and_axial_length(self):
        data = json.loads((ROOT / "art_source/TASK-104/grip/handle_fit_delta.json").read_text(encoding="utf-8"))
        axis = data["axis_mesh_unit"]
        rows = data["vertices_id_original_xyz_new_xyz"]
        self.assertGreater(len(rows), 10000)
        self.assertLess(len(rows), data["source_vertex_count"] / 3)
        for row in rows:
            self.assertLess(row[3], data["head_preserve_min_z_mesh_cm"])
            self.assertLess(abs(sum((row[i+4]-row[i+1])*axis[i] for i in range(3))), .00002)
        self.assertEqual(data["head_displacement_max_cm"], 0)

    def test_corrected_obj_matches_delta_and_has_hand_sized_core(self):
        import hashlib
        path = ROOT / "art_source/TASK-104/grip/stone_bone_axe_handle_fit.obj"
        data = json.loads((path.parent / "handle_fit_delta.json").read_text(encoding="utf-8"))
        self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), data["obj_sha256"])
        lines = path.read_text(encoding="utf-8").splitlines()
        vertices = [[float(v) * scale for v, scale in zip(line.split()[1:], (100, -100, 100))]
                    for line in lines if line.startswith("v ")]
        self.assertEqual(len(vertices), data["source_vertex_count"])
        self.assertEqual(sum(line.startswith("f ") for line in lines), data["source_polygon_count"])
        self.assertGreater(sum(line.startswith("vt ") for line in lines), len(vertices))
        for row in data["vertices_id_original_xyz_new_xyz"]:
            self.assertLess(math.dist(vertices[row[0]], row[4:7]), .000001)
        radii = []
        for vertex in vertices:
            d = [vertex[i]-data["grip_mesh_cm"][i] for i in range(3)]
            t = sum(d[i]*data["axis_mesh_unit"][i] for i in range(3))
            if abs(t) < 9.9 and vertex[2] < -12:
                radii.append(math.sqrt(sum((d[i]-t*data["axis_mesh_unit"][i])**2 for i in range(3))) * .7)
        self.assertGreater(len(radii), 5000)
        self.assertLessEqual(max(radii)*2, 1.8001)

    def test_handle_axis_calibration_matches_archived_mesh_sections(self):
        facts = json.loads((ROOT / "docs/qa/TASK-055/stone-axe-palm-surface-candidate.json").read_text(encoding="utf-8"))["shaft_axis_candidate"]
        lower, upper = [section["centre"] for section in facts["axe_source_slab_centres"]]
        axis = [upper[i] - lower[i] for i in range(3)]
        length = math.sqrt(sum(v*v for v in axis))
        self.assertLess(math.dist([v/length for v in axis], facts["measured_axe_local_tangent_unit"]), 1e-12)
        source = (ROOT / "Source/Hearthward/Equipment/HearthwardAxeGrip.cpp").read_text(encoding="utf-8")
        for v in lower + upper:
            self.assertIn(str(v), source)
        self.assertIn("Hand.GetScale3D() * CharacterScale", source)
        self.assertIn("PalmInHand - Offset / HandWorldScale", source)


if __name__ == "__main__":
    unittest.main()
