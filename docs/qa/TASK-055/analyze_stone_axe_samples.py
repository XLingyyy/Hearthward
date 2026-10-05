"""Offline exact sampled-point projection; no UE or asset/source writes."""
from pathlib import Path
import json
import math
import sys
import numpy as np
import csv

ROOT = Path(sys.argv[1])
OUT = Path(__file__).resolve().parent
EVIDENCE = ROOT / "Saved/Task055/stone-axe-single-node"
FACTS = json.loads((EVIDENCE / "pose-and-axe-facts.json").read_text(encoding="utf-8-sig"))
VERTICES = np.array(json.loads((EVIDENCE / "stone-axe-ue-local-vertices.json").read_text(encoding="utf-8-sig"))["vertices_id_xyz"])
XYZ = VERTICES[:, 1:]


def rotate(quat, point):
    q = np.asarray(quat)
    p = np.asarray(point)
    return p + 2 * (q[3] * np.cross(q[:3], p) + np.cross(q[:3], np.cross(q[:3], p)))


def project_image(point, image):
    camera = image["camera_world"]
    q = camera["rotation_quat_xyzw"]
    camera_local = rotate([-q[0], -q[1], -q[2], q[3]], np.array(point) - camera["translation_cm"])
    scale = 1024 / image["ortho_width_cm"]
    return [float(512 + camera_local[1] * scale), float(512 - camera_local[2] * scale)]


def world_point(tf, point):
    return np.array(tf["translation_cm"]) + rotate(tf["rotation_quat_xyzw"], np.array(point) * tf["scale_xyz"])


slab = XYZ[np.abs(XYZ[:, 2] + 27.5) <= 0.5]
low, high = np.quantile(slab, [0.05, 0.95], axis=0)
GRIP = np.array([(low[0] + high[0]) / 2, (low[1] + high[1]) / 2, -27.5])
POINTS = {"Grip": GRIP, "BladeBase": VERTICES[VERTICES[:, 0] == 41603][0, 1:], "BladeTip": VERTICES[VERTICES[:, 0] == 26958][0, 1:]}
selection = {
    "status": "Measured geometric candidates; grip/rotation/contact acceptance pending root review and actual calibrated render",
    "mesh_path": FACTS["axe_asset"]["path"].split(".")[0], "coordinate_space": "UE mesh local cm",
    "source_facts": str(EVIDENCE / "pose-and-axe-facts.json"), "source_vertices": str(EVIDENCE / "stone-axe-ue-local-vertices.json"),
    "selection_evidence": "All three actual identity renders reviewed. Grip is the wrapped lower-shaft centre at Z=-27.5, using the midpoint of measured 5th/95th percentile X/Y slab coordinates. BladeBase/Tip are actual outer +Y stone cutting-edge lower/upper vertices, distinct from shaft or leather/horn decorations.",
    "candidates": {name: p.tolist() for name, p in POINTS.items()},
    "grip_slab": {"z_min_max_cm": [-28, -27], "vertex_count": len(slab), "q05_xyz_cm": low.tolist(), "q95_xyz_cm": high.tolist(), "centre_is_internal_not_surface_vertex": True},
    "blade_vertices": {"BladeBase": 41603, "BladeTip": 26958},
    "identity_image_projection_pixels": {name: {Path(image["file"]).name: project_image(point, image) for image in FACTS["images"][:3]} for name, point in POINTS.items()},
    "limitations": ["Grip interior selection is a reviewed candidate, not a solved palm transform", "Current 0.7 absolute scale and existing relative rotation retained", "No Content sockets authored", "No native blend 0.12, root/world movement or actual target collision measured"],
}
(OUT / "stone-axe-measured-endpoints.json").write_text(json.dumps(selection, ensure_ascii=False, indent=2), encoding="utf-8")

projected = {"source_captured_utc": FACTS["captured_utc"], "method": "Exact T+QuatRotate(Scale*localPoint) at existing actual sampled transforms; candidate recenter only subtracts Grip before rotation, no runtime action executed", "clips": {}}
for label, clip in FACTS["clips"].items():
    rows = []
    for sample in clip["samples"]:
        tf = sample["actual_held_axe_world"]
        row = {"clip_time_seconds": sample["clip_time_seconds"], "actual_current_binding": {name: world_point(tf, p).tolist() for name, p in POINTS.items()}, "candidate_grip_recenter_existing_rotation": {name: world_point(tf, p - GRIP).tolist() for name, p in POINTS.items()}, "hand_world_cm": sample["actual_world_pose"]["hand_r"]["translation_cm"]}
        if label == "current_attack":
            row["uniform_action_seconds"] = sample["current_uniform_light_action_time_seconds"]
        rows.append(row)
    projected["clips"][label] = rows

attack = projected["clips"]["current_attack"]
length = FACTS["clips"]["current_attack"]["actual_length_seconds"]
base, tip = POINTS["BladeBase"], POINTS["BladeTip"]
edge = []
for z in np.arange(base[2], tip[2] + .001, .5):
    nearby = XYZ[np.abs(XYZ[:, 2] - z) <= .3]
    edge.append(nearby[np.argmax(nearby[:, 1])])
d = tip - base
errors = []
for point in edge:
    fraction = np.clip(np.dot(point - base, d) / np.dot(d, d), 0, 1)
    errors.append(float(np.linalg.norm(point - (base + fraction * d))))
parent = FACTS["clips"]["current_attack"]["samples"][0]["actual_world_pose"]["hand_r"]
relative = FACTS["constructor"]["held_relative"]
relative_offset = -rotate(relative["rotation_quat_xyzw"], GRIP * .7) / parent["scale_xyz"]
metrics = {"sample_count": len(attack), "clip_length_seconds": length,
    "current_active_uniform_clip_seconds": [.35 * length, .53 * length],
    "current_grip_to_hand_error_cm": float(np.linalg.norm(GRIP) * .7),
    "edge_chord_world_length_cm_at_scale_point7": float(np.linalg.norm(d) * .7),
    "edge_outer_contour_max_distance_to_chord_world_cm": max(errors) * .7,
    "recenter_relative_translation_candidate_at_first_actual_parent_scale": relative_offset.tolist(),
    "relative_offset_is_candidate_only": True,
    "diagnostic_target_face": {"x_world_cm": 80, "y_min_max_cm": [-60, -10], "z_min_max_cm": [80, 170], "meaning": "Explicit QA box front face for geometry diagnosis; no live enemy collision or gameplay acceptance"},
    "sampled_edge_face_intersections": {},
    "phase_candidate": {"logical_windup_active_recovery_seconds": [.35, .18, .47], "source_clip_active_seconds": [.6 * length, .8 * length], "source_clip_active_normalized": [.6, .8], "basis": "Includes measured forward/down cutting-edge motion and diagnostic target-face crossings; gameplay test still required"},
}
for binding in ("actual_current_binding", "candidate_grip_recenter_existing_rotation"):
    hits = []
    for row in attack:
        b, t = np.array(row[binding]["BladeBase"]), np.array(row[binding]["BladeTip"])
        if abs(t[0] - b[0]) < 1e-12:
            continue
        alpha = (80 - b[0]) / (t[0] - b[0])
        if not 0 <= alpha <= 1:
            continue
        p = b + alpha * (t - b)
        if -60 <= p[1] <= -10 and 80 <= p[2] <= 170:
            hits.append({"uniform_action_seconds": row["uniform_action_seconds"], "clip_time_seconds": row["clip_time_seconds"], "point_world_cm": p.tolist()})
    metrics["sampled_edge_face_intersections"][binding] = hits
metrics["pose_error_max"] = {name: max(sample["last_pose_errors"][name]["translation_error_cm"] for clip in FACTS["clips"].values() for sample in clip["samples"]) for name in ("hand_r", "head", "pelvis")}
projected["metrics"] = metrics
(OUT / "stone-axe-endpoint-trajectories.json").write_text(json.dumps(projected, ensure_ascii=False, indent=2), encoding="utf-8")

with (OUT / "stone-axe-attack-trajectories.csv").open("w", encoding="utf-8", newline="") as handle:
    writer = csv.writer(handle)
    writer.writerow(["binding", "action_seconds", "clip_seconds", "point", "world_x_cm", "world_y_cm", "world_z_cm"])
    for row in attack:
        for binding in ("actual_current_binding", "candidate_grip_recenter_existing_rotation"):
            for name, xyz in row[binding].items():
                writer.writerow([binding, row["uniform_action_seconds"], row["clip_time_seconds"], name, *xyz])
print(json.dumps(metrics, ensure_ascii=False, indent=2))
