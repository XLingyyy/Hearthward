"""Read-only reproduction of the actual captured LOD0 linear blend skinning."""
from pathlib import Path
import json
import numpy as np

HERE = Path(__file__).resolve().parent
SOURCE = HERE / "stone-axe-palm-skin-inputs-actual.json"
OUTPUT = HERE / "stone-axe-hand-skin-reconstruction.json"


def rotation(q):
    x, y, z, w = q
    return np.array([[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
                     [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
                     [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]])


def transform_matrix(tf):
    m = np.eye(4)
    m[:3, :3] = np.diag(tf["scale_xyz"]) @ rotation(tf["rotation_quat_xyzw"]).T
    m[3, :3] = tf["translation_cm"]
    return m


def skin_positions(vertices, matrices, dtype):
    result = []
    for vertex in vertices:
        p = np.array([*vertex["source_reference_mesh_cm"], 1.], dtype=dtype)
        skinned = np.zeros(3, dtype=dtype)
        for influence in vertex["actual_influences"]:
            weight = dtype(influence["raw_uint16_weight"]) * dtype(1. / 65535.)
            skinned += (p @ matrices[influence["mesh_bone_index"]].astype(dtype))[:3] * weight
        result.append(skinned)
    return np.array(result, dtype=dtype)


def summarize(vertices, expected, actual):
    errors = np.linalg.norm(actual.astype(np.float64)-expected, axis=1)
    index = int(errors.argmax())
    return {"maximum_component_position_error_cm": float(errors[index]),
            "rms_component_position_error_cm": float(np.sqrt(np.mean(errors**2))),
            "worst_lod0_vertex_id": vertices[index]["lod0_vertex_id"],
            "worst_expected_component_cm": expected[index].tolist(),
            "worst_reconstructed_component_cm": actual[index].tolist()}


def main():
    d = json.loads(SOURCE.read_text(encoding="utf-8-sig"))
    vertices = d["lod0_skin_vertices"]
    inputs = {b["mesh_bone_index"]: b for b in d["all_used_influence_bone_skinning_inputs"]}
    required = {w["mesh_bone_index"] for v in vertices for w in v["actual_influences"] if w["raw_uint16_weight"]}
    if required != inputs.keys():
        raise RuntimeError("Actual used influence bone matrices are incomplete or contain unexpected entries")
    if any(sum(w["raw_uint16_weight"] for w in v["actual_influences"]) != 65535 for v in vertices):
        raise RuntimeError("Actual weights do not meet the observed uint16 total; do not renormalize")
    matrices = {i: np.array(b["actual_ref_to_local_matrix_row_major"]).reshape(4, 4) for i, b in inputs.items()}
    derived = {i: np.array(b["inverse_reference_matrix_row_major"]).reshape(4, 4) @ transform_matrix(b["actual_component"]) for i, b in inputs.items()}
    expected = np.array([v["skinned_component_cm"] for v in vertices])
    reconstructed32 = skin_positions(vertices, matrices, np.float32)
    reconstructed64 = skin_positions(vertices, matrices, np.float64)
    from_tf = skin_positions(vertices, derived, np.float64)
    matrix_errors = {b["name"]: float(np.abs(derived[i]-matrices[i]).max()) for i, b in inputs.items()}
    names = {b["name"]: b for b in inputs.values()}
    report = {"meaning": "Original pose reconstruction only; no candidate, contact, animation author or UE execution",
              "actual_capture": str(SOURCE), "actual_source_pose_seconds": d["actual_single_node_time_seconds"],
              "vertices": len(vertices), "triangles": len(d["triangles_fully_inside_selected_vertices"]), "used_bones": len(inputs),
              "method": "Original LOD0 reference positions; all actual raw weights / 65535; homogeneous row vectors times recorded FMatrix44f RefToLocal",
              "actual_ref_to_local_float32": summarize(vertices, expected, reconstructed32),
              "actual_ref_to_local_float64": summarize(vertices, expected, reconstructed64),
              "inverse_reference_times_component_tf_float64": summarize(vertices, expected, from_tf),
              "maximum_matrix_entry_difference_by_bone": matrix_errors,
              "five_02_parent01_data_available": all(f"{p}_01_r" in names and f"{p}_02_r" in names and f"{p}_03_r" in names for p in ("index", "middle", "ring", "pinky", "thumb")),
              "maximum_world_position_error_cm": float(np.linalg.norm(
                  np.array([v["skinned_world_cm"] for v in vertices])-
                  np.c_[reconstructed32, np.ones(len(vertices))] @ transform_matrix(d["mesh_world"])[:, :3], axis=1).max()),
              "limits": ["No candidate has been computed or accepted; actual skin subset is open.",
                         "float32 and float64 are arithmetic diagnostics against the same frozen input; neither changes the recorded data."]}
    OUTPUT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
