"""Root-run public UE transient mesh for the measured local-section candidate.

No AssetTools, package creation, asset registration or save APIs are called.
The source cached description is borrowed within one generator step, restored
in finally, and never passed through the source asset's build/commit methods.
"""
import json
from pathlib import Path
import unreal


def build_local_section_transient(source, candidate, evidence):
    source_package = source.get_path_name().split(".", 1)[0]
    dirty_names = lambda: [p.get_path_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
    if source_package in dirty_names():
        raise RuntimeError("Local-section QA requires the freshly loaded source package to be clean")
    positions = json.loads((Path(__file__).resolve().parent / Path(candidate["vertex_position_candidate_file"]).name).read_text(encoding="utf-8-sig"))
    if source.get_path_name() != candidate["mesh"]["path"] or source.get_path_name() != positions["mesh"]["path"]:
        raise RuntimeError("Candidate source asset identity differs")
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    description = source.get_static_mesh_description(0)
    if description is None or description.get_vertex_count() != positions["original_vertex_count"]:
        raise RuntimeError("Actual source description/vertex count differs from measured candidate")
    edits = []
    for index, row in enumerate(positions["vertices_id_original_xyz_new_xyz"]):
        vertex_id = unreal.VertexID(id_value=int(row[0]))
        if not description.is_vertex_valid(vertex_id):
            raise RuntimeError("Actual measured source VertexID is invalid")
        original = description.get_vertex_position(vertex_id)
        if (original-unreal.Vector(*row[1:4])).length() > .00001:
            raise RuntimeError("Actual source vertex differs from the measured candidate baseline")
        edits.append((vertex_id, original, unreal.Vector(*row[4:7])))
        if (index+1) % 2000 == 0:
            yield lambda: True
    temporary = unreal.new_object(unreal.StaticMesh, name="Task055LocalSectionTransient")
    if not temporary.get_path_name().startswith("/Engine/Transient."):
        raise RuntimeError("Candidate static mesh is outside the transient package")
    temporary.set_editor_property("static_materials", source.get_editor_property("static_materials"))
    evidence.update({"source_asset": source.get_path_name(), "source_package_clean_before": True,
                     "transient_mesh": temporary.get_path_name(), "actual_source_vertex_count": description.get_vertex_count(),
                     "actual_source_triangle_count": description.get_triangle_count(), "changed_vertex_count": len(edits),
                     "build_api": "StaticMesh.build_from_static_mesh_descriptions, then StaticMeshEditorSubsystem.set_lod_build_settings",
                     "source_render_rebuilt": False, "saved_assets": [], "asset_registry_registration": False})
    # Do not yield between source mutation and restoration. Public Build commits
    # a by-value description copy into the transient mesh before returning.
    try:
        for vertex_id, original, changed in edits:
            description.set_vertex_position(vertex_id, changed)
        # Use editor build, not fast build: fast build latches bDoFastBuild and
        # would bypass normal/tangent recomputation on the subsequent rebuild.
        temporary.build_from_static_mesh_descriptions([description], False, False)
    finally:
        for vertex_id, original, changed in edits:
            description.set_vertex_position(vertex_id, original)
        error = max((description.get_vertex_position(i)-p).length() for i, p, _ in edits)
        evidence["source_geometry_restore_max_error_cm"] = error
        evidence["source_package_dirty_after_restore_before_cleanup"] = source_package in dirty_names()
        if source_package in dirty_names():
            if not unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).set_dirty_flag(source, False):
                raise RuntimeError("Could not restore clean source package state after restored temporary cache edit")
        evidence["source_package_clean_after_restore"] = source_package not in dirty_names()
        if error != 0.0 or not evidence["source_package_clean_after_restore"]:
            raise RuntimeError("Actual source description or package state did not restore")
    editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    options = editor.get_lod_build_settings(source, 0)
    options.set_editor_property("recompute_normals", True)
    options.set_editor_property("recompute_tangents", True)
    options.set_editor_property("generate_lightmap_u_vs", False)
    options.set_editor_property("remove_degenerates", False)
    evidence["transient_build_options_requested"] = {"recompute_normals": True, "recompute_tangents": True,
                                                    "generate_lightmap_u_vs": False, "remove_degenerates": False}
    # This public setter invokes PostEditChange/rebuild on the transient object.
    editor.set_lod_build_settings(temporary, 0, options)
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    yield lambda: True
    actual_options = editor.get_lod_build_settings(temporary, 0)
    evidence["transient_build_options_readback"] = {name: bool(actual_options.get_editor_property(name)) for name in evidence["transient_build_options_requested"]}
    sockets = {}
    for name in ("Grip", "BladeBase", "BladeTip"):
        original = source.find_socket(name)
        if original is None:
            raise RuntimeError("Actually authored source socket missing: " + name)
        copy = unreal.new_object(unreal.StaticMeshSocket, outer=temporary)
        for field in ("socket_name", "relative_location", "relative_rotation", "relative_scale", "tag"):
            copy.set_editor_property(field, original.get_editor_property(field))
        temporary.add_socket(copy)
        actual_socket = temporary.find_socket(name)
        point = actual_socket.get_editor_property("relative_location")
        if (point-original.get_editor_property("relative_location")).length() != 0.:
            raise RuntimeError("Transient socket differs from actual source socket")
        sockets[name] = [float(point.x), float(point.y), float(point.z)]
    evidence["source_and_transient_socket_locations_cm"] = sockets
    actual_description = temporary.get_static_mesh_description(0)
    actual_error = max((actual_description.get_vertex_position(i)-p).length() for i, _, p in edits)
    evidence["transient_candidate_vertex_max_error_cm"] = actual_error
    evidence["transient_source_triangle_count"] = actual_description.get_triangle_count()
    evidence["transient_render_triangle_count"] = temporary.get_num_triangles(0)
    evidence["source_package_clean_before_render"] = source_package not in dirty_names()
    if actual_error > .00001 or not evidence["source_package_clean_before_render"]:
        raise RuntimeError("Transient candidate geometry differs or source package became dirty")
    return temporary
