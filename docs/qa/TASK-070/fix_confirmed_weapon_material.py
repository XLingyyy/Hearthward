"""Root-run mutation draft. No automatic call; pass only probe-confirmed channels."""

import json
from pathlib import Path

import unreal


DEFAULT_MANIFEST = Path(__file__).with_name("first-weapon-import-manifest.json")
CHANNELS = {
    "BaseColor": ("Color", unreal.MaterialProperty.MP_BASE_COLOR, ""),
    "Normal": ("Normal", unreal.MaterialProperty.MP_NORMAL, ""),
    "Roughness": ("Roughness", unreal.MaterialProperty.MP_ROUGHNESS, "R"),
    "Metallic": ("Metallic", unreal.MaterialProperty.MP_METALLIC, "R"),
}


def apply_material_fix(piece, confirmed_channels, manifest_path=DEFAULT_MANIFEST):
    """Call after root registers/locks packages and authorizes observed PBR fixes.

    Does not import a missing texture, create another material, rewrite other
    material inputs, change mesh geometry/scale/sockets, or assign components.
    """
    channels = list(dict.fromkeys(confirmed_channels))
    if not channels or any(channel not in CHANNELS for channel in channels):
        raise ValueError("Specify one or more confirmed BaseColor/Normal/Roughness/Metallic defects.")
    manifest = json.loads(Path(manifest_path).read_text(encoding="utf-8-sig"))
    entry = next(item for item in manifest["imports"] if item["piece"] == piece)
    material = unreal.load_asset(entry["expected_material_package"])
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Expected an existing UMaterial; inspect the actual class before choosing a different fix.")
    if material.get_editor_property("use_material_attributes"):
        raise RuntimeError("MaterialAttributes graph requires separate review; direct PBR rewiring is not authorized here.")
    textures = {}
    for name in ("Color", "Normal", "Roughness", "Metallic"):
        texture = unreal.load_asset(entry["destination"] + "/" + name)
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError("Missing/wrong texture " + name + "; root must import it through public UEClient.assets first.")
        if texture.get_editor_property("virtual_texture_streaming"):
            raise RuntimeError("Unexpected virtual texture " + name + "; inspect before applying this non-VT sample fix.")
        textures[name] = texture
    lib = unreal.MaterialEditingLibrary
    expressions = list(lib.get_material_expressions(material))
    report = {"piece": piece, "material": material.get_path_name(), "requested_channels": channels, "changes": []}
    touched_textures = []
    for index, channel in enumerate(channels):
        texture_name, prop, output_name = CHANNELS[channel]
        texture = textures[texture_name]
        compression = texture.get_editor_property("compression_settings")
        before = {
            "srgb": bool(texture.get_editor_property("srgb")),
            "compression_settings": str(compression),
            "input_node": str(lib.get_material_property_input_node(material, prop)),
            "output_name": lib.get_material_property_input_node_output_name(material, prop),
        }
        wanted_srgb = channel == "BaseColor"
        wanted_compression = compression
        if channel == "Normal":
            wanted_compression = unreal.TextureCompressionSettings.TC_NORMALMAP
        elif channel in ("Roughness", "Metallic") and compression not in (
            unreal.TextureCompressionSettings.TC_DEFAULT, unreal.TextureCompressionSettings.TC_MASKS
        ):
            wanted_compression = unreal.TextureCompressionSettings.TC_MASKS
        elif channel == "BaseColor" and compression in (
            unreal.TextureCompressionSettings.TC_NORMALMAP, unreal.TextureCompressionSettings.TC_MASKS
        ):
            wanted_compression = unreal.TextureCompressionSettings.TC_DEFAULT
        if texture.get_editor_property("srgb") != wanted_srgb:
            texture.set_editor_property("srgb", wanted_srgb)
            touched_textures.append(texture)
        if compression != wanted_compression:
            texture.set_editor_property("compression_settings", wanted_compression)
            touched_textures.append(texture)
        if channel == "Normal":
            sampler = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
        elif channel == "BaseColor":
            sampler = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
        elif wanted_compression == unreal.TextureCompressionSettings.TC_MASKS:
            sampler = unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
        else:
            sampler = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
        old_node = lib.get_material_property_input_node(material, prop)
        candidates = [node for node in expressions if isinstance(node, unreal.MaterialExpressionTextureSample)
                      and node.get_editor_property("texture") == texture]
        sample = old_node if old_node in candidates else (candidates[0] if candidates else None)
        created = sample is None
        if created:
            sample = lib.create_material_expression(material, unreal.MaterialExpressionTextureSample, -400, index * 220)
            if sample is None:
                raise RuntimeError("Cannot create texture sample for " + channel)
            sample.set_editor_property("texture", texture)
            expressions.append(sample)
        sample.set_editor_property("sampler_type", sampler)
        if not lib.connect_material_property(sample, output_name, prop):
            raise RuntimeError("Cannot connect " + channel)
        report["changes"].append({
            "channel": channel, "texture": texture.get_path_name(), "before": before,
            "sample": sample.get_path_name(), "sample_created": created,
            "srgb": wanted_srgb, "compression_settings": str(wanted_compression),
            "sampler_type": str(sampler), "output_name": output_name,
        })
    errors = list(lib.recompile_material(material))
    report["compiler_errors"] = errors
    if errors:
        raise RuntimeError("Material compile failed; no explicit save performed: " + "; ".join(errors))
    for texture in dict.fromkeys(touched_textures):
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
            raise RuntimeError("Cannot save texture " + texture.get_path_name())
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError("Cannot save material " + material.get_path_name())
    report["saved"] = True
    unreal.log("TASK070 confirmed PBR fix: " + json.dumps(report, ensure_ascii=False))
    return report


def apply_instance_texture_fix(piece, confirmed_bindings, manifest_path=DEFAULT_MANIFEST):
    """Root-authorized exact MIC texture overrides using existing parent parameters.

    confirmed_bindings maps actual global parent parameter names from the probe
    to Color/Normal/Roughness/Metallic. No inferred parameter names or parent edits.
    """
    if not confirmed_bindings or any(name not in ("Color", "Normal", "Roughness", "Metallic") for name in confirmed_bindings.values()):
        raise ValueError("Specify probe-confirmed existing parameter -> imported texture bindings.")
    manifest = json.loads(Path(manifest_path).read_text(encoding="utf-8-sig"))
    entry = next(item for item in manifest["imports"] if item["piece"] == piece)
    instance = unreal.load_asset(entry["expected_material_package"])
    if not isinstance(instance, unreal.MaterialInstanceConstant):
        raise RuntimeError("Expected the actual imported MaterialInstanceConstant.")
    parent = instance.get_editor_property("parent")
    if parent is None:
        raise RuntimeError("Missing parent; inspect the actual imported instance before fixing overrides.")
    lib = unreal.MaterialEditingLibrary
    existing_names = {str(name) for name in lib.get_texture_parameter_names(parent)}
    if any(name not in existing_names for name in confirmed_bindings):
        raise ValueError("Requested texture parameter is not declared by the existing parent.")
    textures = {}
    for parameter_name, texture_name in confirmed_bindings.items():
        texture = unreal.load_asset(entry["destination"] + "/" + texture_name)
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError("Missing/wrong texture " + texture_name + "; use public UEClient.assets.import_texture first.")
        textures[parameter_name] = texture
    report = {"piece": piece, "instance": instance.get_path_name(), "parent": parent.get_path_name(), "changes": []}
    for parameter_name, texture in textures.items():
        previous = lib.get_material_instance_texture_parameter_value(instance, parameter_name)
        # UE5.8 implementation updates the instance but leaves its returned bool false.
        # Record it; verify the actual override through the public getter instead.
        returned = lib.set_material_instance_texture_parameter_value(instance, parameter_name, texture)
        actual = lib.get_material_instance_texture_parameter_value(instance, parameter_name)
        overridden = lib.is_material_instance_parameter_overridden(instance, parameter_name)
        if actual != texture or not overridden:
            raise RuntimeError("MIC override readback failed for " + parameter_name)
        report["changes"].append({
            "parameter_name": parameter_name,
            "before": previous.get_path_name() if previous else None,
            "after": actual.get_path_name(),
            "setter_returned": bool(returned),
            "instance_override_readback": bool(overridden),
        })
    report["parent_after"] = instance.get_editor_property("parent").get_path_name()
    if not unreal.EditorAssetLibrary.save_loaded_asset(instance):
        raise RuntimeError("Cannot save MIC " + instance.get_path_name())
    report["saved_instance_only"] = True
    unreal.log("TASK070 confirmed MIC texture fix: " + json.dumps(report, ensure_ascii=False))
    return report


if __name__ == "__main__":
    unreal.log("TASK070 material-fix draft loaded. Root must explicitly call the function matching the actual asset class and confirmed defects.")
