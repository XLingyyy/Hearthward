"""Bounded delivery evidence inventory; never starts UE or reads user saves/models."""
from pathlib import Path
import json
import os
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[3]
OUTPUT = Path(__file__).resolve().parent
BASE = "6fcf5c22e965f0f7409438f19bc7b09e96ffb058"
MODEL_REPORTS = {
    "cpu": "docs/qa/TASK-087/cpu-final-20261007-01.json",
    "vulkan": "docs/qa/TASK-087/vulkan-final-20261007-02.json",
}
SCENE_REPORTS = {
    backend: f"docs/qa/TASK-102/settled-dialogue-{backend}-20261007-01/summary.json"
    for backend in ("cpu", "vulkan")
}
LATEST_NATIVE = ".agent-local/qa/TASK-099/native-final-audio-20261007-13"
PACKAGE = ".agent-local/qa/TASK-103/package-20261007-5"
BUILD_INFO = "docs/releases/iteration-084-103-rc/CANDIDATE5_BUILD_INFO.json"
OS3 = ".agent-local/qa/TASK-103/os-launchers-candidate3-20261007/results.json"
OS4 = "docs/qa/TASK-103/OS_CANDIDATE4_PARTIAL_NORMAL_INPUT.json"
NAVIGATION = "docs/qa/TASK-103/NAV12_GROUNDED_FINAL_FIRST_BLOCKER.json"
OS5 = "docs/qa/TASK-103/OS_CANDIDATE5_PARTIAL_NORMAL_INPUT.json"
NAV_HISTORY = "docs/qa/TASK-103/NAV09_11_PROJECTION_HISTORY.json"
NAVIGATION07 = ".agent-local/qa/TASK-084/fresh-prologue-navigation-api-20261007-07"
NAVIGATION06 = ".agent-local/qa/TASK-084/fresh-prologue-navigation-api-20261007-06"
NAVIGATION05 = ".agent-local/qa/TASK-084/fresh-prologue-navigation-api-20261007-05"
NAVIGATION04 = ".agent-local/qa/TASK-084/fresh-prologue-navigation-api-20261007-04"


def read_json(relative):
    return json.loads((ROOT / relative).read_text(encoding="utf-8-sig"))


def write_json(name, value):
    (OUTPUT / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    reports = [
        ".agent-local/qa/TASK-085-088/native-baseline-20261007/index.json",
        ".agent-local/qa/TASK-085-099/native-first-20261007/index.json",
        ".agent-local/qa/TASK-088-101/native-integration-20261007/index.json",
        ".agent-local/qa/TASK-090-100/native-red-20261007/index.json",
        ".agent-local/qa/TASK-090-100/native-green-20261007/index.json",
        ".agent-local/qa/TASK-087-099/native-green-20261007-01/index.json",
        ".agent-local/qa/TASK-099/native-green-20261007-03/native/index.json",
        ".agent-local/qa/TASK-099/native-pre-contact-20261007-06/native/index.json",
        ".agent-local/qa/TASK-099/native-post-contact-20261007-07/native/index.json",
        ".agent-local/qa/TASK-099/native-save-isolated-20261007-08/native/index.json",
        f"{LATEST_NATIVE}/native/index.json",
        ".agent-local/qa/TASK-091/native-stair-top-20261007-red/native/index.json",
        ".agent-local/qa/TASK-091/native-stair-top-20261007-green/native/index.json",
        ".agent-local/qa/TASK-091/native-side-gate-20261007-red/native/index.json",
        ".agent-local/qa/TASK-091/native-side-gate-20261007-green/native/index.json",
    ]
    original_copy = read_json("docs/qa/TASK-101/OS_ORIGINAL_COPY_COMPATIBILITY.json")
    original_copy4 = read_json("docs/qa/TASK-101/OS_CANDIDATE4_ORIGINAL_COPY_COMPATIBILITY.json")
    build_info = read_json(BUILD_INFO)
    document_install = read_json(f"{PACKAGE}/document-install.json")
    os3 = read_json(OS3)
    os4 = read_json(OS4)
    navigation = read_json(NAVIGATION)
    navigation_history = read_json(NAV_HISTORY)
    os5 = read_json(OS5)
    finalization_path = f"{PACKAGE}/finalization-result.json"
    finalization = read_json(finalization_path) if (ROOT / finalization_path).is_file() else None
    zip_state = finalization["status"] if finalization else "NOT_CREATED"
    distribution_hashes = ([{"algorithm": "SHA256", "value": finalization["sha256"],
                            "source": finalization_path, "computations": finalization["sha256_computations"]}]
                           if finalization and finalization["status"] == "INTERNAL_CANDIDATE_ZIP_CREATED" else [])
    navigation06 = read_json(f"{NAVIGATION06}/results.json")
    navigation07 = read_json(f"{NAVIGATION07}/results.json")
    navigation05 = read_json(f"{NAVIGATION05}/results.json")
    nav_read = navigation05["nav_diagnostics"][0]
    candidate5_prep = read_json("docs/qa/TASK-103/CANDIDATE5_PREPARATION.json")
    navigation04 = read_json(f"{NAVIGATION04}/results.json")
    latest_native = read_json(f"{LATEST_NATIVE}/native/index.json")
    native_result = read_json(f"{LATEST_NATIVE}/result.json")
    audio = read_json("Resources/Data/experience.json")
    events = audio["sound_events"]
    wavs = sorted({item["file"] for item in events})
    models = {}
    for backend, relative in MODEL_REPORTS.items():
        report = read_json(relative)
        models[backend] = {
            "source_report": relative, "run_id": report["run_id"],
            "source_revision": report["source_revision"], "backend": report["backend"],
            "diagnostic_subset": report["diagnostic_subset"],
            "original_denominators": report["original_denominators"],
            "summary": report["summary"], "quality": "FAIL" if not report["ok"] else "PASS",
            "original_warm": report["warm_component_latency"],
            "warm_with_predeclared_auxiliary": report["warm_component_latency_with_auxiliary"],
        }
    scenes = {}
    for backend, relative in SCENE_REPORTS.items():
        report = read_json(relative)
        frames = report["frame_report"]
        scenes[backend] = {
            "source_report": relative, "status": report["status"], "scene": report["scene"],
            "evidence_level": report["evidence_level"], "build_source_binding": report["build_source_binding"],
            "csv": report["csv_archive"],
            "frames": {key: frames[key] for key in ("numeric_frames", "seconds", "p99_ms", "one_percent_low_fps", "over_50ms", "over_50ms_fraction")},
            "frame_gates": report["diagnostic_frame_gates"], "model": report["model"],
            "resources": report["resources"], "process_exit_verified": report["process_exit_verified"],
            "six_scene_joint_acceptance": report["six_scene_joint_acceptance"],
            "shipping": report["shipping"], "normal_os_input": report["normal_os_input"],
        }
    native = {}
    for relative in reports:
        report = read_json(relative)
        for test in report["tests"]:
            native[test["fullTestPath"]] = {
                "source_report": relative,
                "report_created_on": report["reportCreatedOn"],
                "test": test["fullTestPath"],
                "state": test["state"],
                "warnings": test["warnings"],
                "errors": test["errors"],
            }
    reuse = {
        95: ["Hearthward.Equipment055.HeldAxeFollowsCurrentInstanceAndMode",
             "Hearthward.Equipment055.StoneAxeLightUsesClipPhaseAndPhysicalBlade",
             "Hearthward.Equipment055.StoneAxeHeavyUsesCurrentMoveAndPhysicalBlade"],
        96: ["Hearthward.Hometown077.BedroomAndEscapeClearance"],
        97: ["Hearthward.Animals.FrameBoundaryAndEscape",
             "Hearthward.Farming062.IndividualGrowthAndProductProgress",
             "Hearthward.Farming062.CropGeometryConsumesCalendarStage"],
        98: ["Hearthward.Camp059.WorkerAssignmentPresentation",
             "Hearthward.Farming062.CropProgressAndHarvestCapacity",
             "Hearthward.Farming062.PenProductiveAnimalAndPausePresentation"],
        100: [x for x in native if x.startswith(("Hearthward.Campaign049.", "Hearthward.Campaign067."))],
        101: ["Hearthward.Save.ActualLoadPointInvalidatesOldCommandTicket",
              "Hearthward.Save.ActualLoadPointPreservesPartialMaterialSettlementRevision",
              "Hearthward.Save.CompatibilityPreviewAndConsent", "Hearthward.Save.FileIntegrityAndSnapshot",
              "Hearthward.Time.DomainPartitionRefreshAndEpoch", "Hearthward.UI069.LoadingRestoresLatestPageInputMode",
              "Hearthward.Camp.PartitionSleepAndRestoration"],
    }
    implementations = {
        84: "Nav12普通API四节点完成；实际确认地形+Z100/固定XY250/Z220，首段完整query/controller路径普通移动1976.913883cm；第二路径valid=true/partial=true在SimpleMove前停止，自有退出true，无完整撤离/营地/首救/新保存/OS。09—11 QA插值Z与只读诊断保留，不定地图/碰撞根因。",
        85: "只读类型化个人/队伍显示契约和原动作适配已实现。",
        86: "个人/队伍状态卡、真实停工原因与恢复入口已接入。",
        87: "生命周期3项与真实候选/各档身份投影4项已GREEN；固定AI Source.2原60双后端均已实际完成且语义质量FAIL；历史完整/定位失败独立保留。",
        88: "配方搜索/实际可制作筛选/epoch材料目标；首轮界面夹具断言已核实。",
        89: "装备实例GUID/真实转移预览/操作卡幂等/距设施与epoch拒绝。",
        90: "准备视图/日志地点和双方倒地优先提示已接入；最新3项原生GREEN。",
        91: "顶/侧门各独立同case Native RED1F0W1E→GREEN1P0W0E，四原件不合分母；Nav08—12实际四节点交接，12末段首段19.77m实际移动，第二partial停止；OS完整路线/渲染/传送未验。",
        92: "明确waiting停止真实AI路径的RED→最小修复→Fixture GREEN；正式路线未验。",
        93: "营地准备/本地设施/旧tier卡/会话反馈；先前2条失败修正诊断后2项Success，Receipt本轮Success。",
        94: "845行精确/候选/未知来源资产台账与逐包责任。未替换资产。",
        95: "技术绑定调查及既有3条动作回归；未更换角色/武器资产。",
        96: "石堡结构/材质生命周期/碰撞职责调查及原077净空回归。未更换资产。",
        97: "14物种303动作与别名/作物状态/来源调查及既有3条行为回归。未更换资产。",
        98: "8设施Kind/碰撞/岗位与187 UI定义文件/UV调查及既有3条行为回归。未更换资产。",
        99: "仓储、真实玩家/NPC/Progress/Combat事件、空挥与持续水源PCM绑定已实施；四条Walk/Run精确包实际写入16个脚步Notify。Native13本单23项23P/0warning/0error，包含实际clip派发/ground门禁与loaded水网格关联、连续PCM、暂停/Load/退出；Native06等真实失败历史保留，实际声音听感未验。",
        100: "127行稳定ID覆盖；未知side/waiting真实RED→091展示修复→3夹具GREEN；7原Campaign回归Success。",
        101: "支持schema/真实隔离文件、Load与原时间分区回归；未改Save格式。",
        102: "CPU/Vulkan单一稳定新游戏固定视角场景已采完整CSV与一条真实模型请求；Development/API层，GPU语义FAIL、CPU HTTP超时，六场景及Shipping/OS联合验收未完成。",
    }
    owner = {
        94: "DEFERRED_DESIGN：070人物/营地/地表样图与衍生首件签收；077石堡方向已有确认。",
        95: "DEFERRED_DESIGN：角色服装/轮廓、武器持握、倒地复活/弓首件审看。",
        96: "DEFERRED_DESIGN：已有石堡方向下Lit近景/补面/夜袭火烟首件审看。",
        97: "DEFERRED_DESIGN：boar→pig外形衍生、作物成熟辨识、自然环境首件审看。",
        98: "DEFERRED_DESIGN：用途可辨设施与关键武器图标首件审看。",
        99: "DEFERRED_DESIGN：实际声音听感；固定录音仍UNPRODUCED，字幕预览交付需Owner决定。",
        100: "DEFERRED_DESIGN：批准095—099资产后既有一区空间/近远景首件；未自批新布局。",
    }
    rows = []
    for number in range(84, 103):
        task_id = f"TASK-{number:03d}"
        task = read_json(f"docs/tasks/{task_id}.json")
        names = [x for x in native if f"Task{number:03d}." in x] + reuse.get(number, [])
        tests = [native[x] for x in dict.fromkeys(names) if x in native]
        report_path = f"docs/qa/{task_id}/REPORT.md"
        rows.append({
            "task": task_id, "title": task["title"], "status_at_audit": task["status"],
            "reference_head": BASE, "implementation_sha": None, "uncommitted_source": True,
            "implementation": implementations[number],
            "source_report": report_path if (ROOT / report_path).is_file() else None,
            "assets": "静态候选/实际Registry来源可定位；candidate5实际Cook SUCCESS，未替换本单Content，逐资产许可闭合及Owner首件NOT_RUN" if 94 <= number <= 98
                      else "当前17个event/12个WAV与CC0来源已登记；candidate5 install核9项许可与water配对；最终有界扫描由外部record独立；路径/存在不是全部Content许可结论；实听/Owner与未知Content许可未验；最终Archive闭合见外部record" if number == 99
                      else "无本单Content变更；运行树闭合独立记录，不补本单完整验收",
            "native": {"layer": "DEVELOPMENT_EDITOR_SELECTED_DIAGNOSTIC_SNAPSHOTS_NOT_SHIPPING_INPUT",
                       "found": len(tests), "success": sum(x["state"] == "Success" for x in tests),
                       "failed": sum(x["state"] == "Fail" for x in tests),
                       "warnings": sum(x["warnings"] for x in tests),
                       "errors": sum(x["errors"] for x in tests), "tests": tests,
                       "status": "RUN_PARTIAL_SCOPE" if tests else "NOT_RUN_OR_NO_NATIVE_SCOPE"},
            "normal_input_evidence": "docs/qa/TASK-101/OS_ORIGINAL_COPY_COMPATIBILITY.json" if number == 101 else OS5 if number == 84 else None,
            "invalid_return_continue_claim": "docs/qa/TASK-103/OS_CONTINUE_ERRATUM.json" if number in (84, 101) else None,
            "normal_input": "PARTIAL候选5/.4：双CMD隐藏脚本启动＋正常OS输入、双Title.4、fresh profile无旧QA档复制；CPU新游戏/main01/Tneutral/F6鼠标保存1→2，Vulkan明确鼠标Continue同卧室/main01与原02:26manual/02:25auto仍2/Tneutral，两后端正常退出全自有进程gone，9截图；本轮模型/中文/IME/旧档兼容/全路线NOT_RUN"
                            if number == 84 else "PARTIAL候选2：J main01地点与准备文本；其他HUD/倒地/正常路径未验"
                            if number == 90 else "PARTIAL候选2/.1：readonly原档copy隔离件列4旧节点/实际Load最新营地，保存5/Return误新游戏6不计Continue/明确重Load营地存7/第三进程鼠标Continue同营地仍7；候选4原档隔离copy列4旧节点/最新户外营地准备任务旧节点Load→手工5→第二进程明确鼠标Continue仍5/6图，仅可见字段。候选5卧室Continue不补旧档最终兼容或四新阶段"
                            if number == 101 else "NOT_RUN：本单正式正常输入验收未覆盖",
            "real_model": "COMPLETE_QUALITY_FAIL：Source.2 CPU原始33/60、受限3/20、明确E2E33/40、执行25/30；Vulkan原始32/60、受限2/20、明确E2E33/40、执行24/30；各边界20/20、白得0。每后端原59暖样本不足，预声明性能辅助1条补足60，CPU组件p95 22.656s≤30s、Vulkan8.672s≤10s；UIpaint/IME/Shipping未验，历史失败单列。"
                          if number == 87 else "CAPTURE_COMPLETE_MODEL_FAILED：单场景Vulkan正常HTTP返回后TARGET_REQUIRED→clarify/无candidate；CPU HTTP120.007s超时→MODEL_UNAVAILABLE/raw空、语义不可评估；各generation1且完整请求在CSV内，无确认/执行"
                          if number == 102 else "NOT_RUN_OR_OUTSIDE_NARROW_NATIVE_SCOPE",
            "real_model_evidence": models if number == 87 else scenes if number == 102 else None,
            "performance": "SINGLE_SCENE_JOINT_FAIL：Vulkan6183帧/61.1867968s/p99 16.7755ms/1%Low49.0051591fps/>50ms0；CPU11603帧/129.8656161s/p99 16.5324ms/1%Low51.3488456fps/>50ms2。两后端1%Low失败，原六稳定场景NOT_RUN；冷启动/请求重叠/低RAM观察分开，非OOM归因。" if number == 102 else "WARM_COMPONENT_ONLY_PASS：CPU22.656s≤30s、Vulkan8.672s≤10s，每后端60暖样本含独立辅助1条，非联合/UI绘制PASS" if number == 87 else "NOT_RUN",
            "owner": owner.get(number, "NOT_RUN：当前实机/视觉签收未验；无新增设计决定待问"),
            "second_machine": "NOT_RUN", "human_samples": "NOT_RUN",
            "shipping": f"BUILD_SUCCESS：{build_info['candidate_id']}/{build_info['proposed_version']}准确冻结/实际BuildCookStageArchive Exit0；OS={os5['state']}（双CMD脚本启动＋OS开局/保存Continue，模型/IME/全路线未验）；ZIP={zip_state}，外部record独立，包内pre-ZIP元数据不代填；Native13仅自身Development快照。",
        })
    write_json("DELIVERY_MATRIX.json", {
        "schema_version": 1, "audited_at_utc": datetime.now(timezone.utc).isoformat(),
        "candidate_id": build_info["candidate_id"],
        "candidate_version_proposed": build_info["proposed_version"],
        "candidate_directory": build_info["candidate_directory"],
        "planned_zip": build_info["planned_zip"],
        "reference_head": BASE, "final_source_freeze": build_info["source_freeze"],
        "final_source_freeze_evidence": build_info["source_freeze_evidence"],
        "final_build": build_info["build_cook_stage_archive"],
        "package_result": f"{PACKAGE}/result.json",
        "package": {"uat_exit_code": build_info["package_uat_exit_code"],
                    "build_seconds": build_info["package_build_seconds"],
                    "cook_seconds": build_info["package_cook_seconds"],
                    "stage_seconds": build_info["package_stage_seconds"],
                    "archive_seconds": build_info["package_archive_seconds"],
                    "cook_errors": build_info["package_cook_errors"],
                    "cook_warnings": build_info["package_cook_warnings"],
                    "cook_warning": build_info["package_cook_warning"],
                    "total_uat_seconds": build_info["package_total_seconds"], "configuration": build_info["configuration"]},
        "delivery_metadata_state": f"BUILD_SUCCESS_PARTIAL_OS_VERIFIED; runtime pre-ZIP snapshot; external finalization={zip_state}",
        "document_install": {"source_report": f"{PACKAGE}/document-install.json",
                             "state": document_install["state"], "file_count": len(document_install["files"]),
                             "hashes_computed": document_install["hashes_computed"],
                             "bounded_text_secret_scan": "BOUNDED_TEXT_CLEAN_AT_INSTALL_AND_FINALIZATION; actual bounded-secret-scan.json retained"},
        "candidate_build_info": BUILD_INFO,
        "shipping_normal_input": os5["state"],
        "shipping_normal_input_evidence": OS5,
        "shipping_os_partial": os5,
        "formal_acceptance_passed": False,
        "finalization_external_summary": "docs/qa/TASK-103/CANDIDATE5_FINALIZATION.json",
        "zip": zip_state, "finalization_result": finalization, "finalization_source": finalization_path,
        "distribution_hashes": distribution_hashes, "runtime_build_info_zip": build_info["zip"],
        "native_source_reports": reports,
        "real_model_reports": MODEL_REPORTS,
        "single_scene_performance_reports": SCENE_REPORTS,
        "mcp": {
            "source_doc": "docs/qa/MCP/20261007/SETUP.md", "source_connection": "docs/qa/MCP/20261007/connection.json",
            "configuration": "UE5.8.2 native ModelContextProtocol/AllToolsets; project/workspace Codex config uses loopback8000/mcp",
            "historical_read_only_connection": "VERIFIED: initialize/initialized/tools/list/SceneTools Bootstrap query/Codex configuration discovery",
            "current_turn_native_tool_mount": "NOT_VERIFIED; existing turn lacks Unreal MCP tool entries, configuration is not a hot-load result",
            "current_endpoint_liveness": "Root actual final resident check passed; depends on Editor life, not chat catalog hot-mount",
            "final_resident_evidence": "docs/qa/MCP/20261007/final-resident-connection.json",
            "final_resident_safe_summary": read_json("docs/qa/MCP/20261007/final-resident-connection.json"),
            "final_resident_connection": "Actual alive-at-check PID52580/startup49.3768s/initialize/initialized/tools-list3meta-tools/readonlyBootstrap/publicRC+Python/CLIenabled match verified by root",
            "cook_dependency": "AllToolsets→GameFeaturesToolset→GameFeatures; official empty-directory GameFeatureData AlwaysCook rule; candidate1 failure and candidate2 retry success remain distinct",
            "eula_warning": "RETAINED; no legal approval or provenance closure inferred",
        },
        "native_process_diagnostics": {
            "source_result": f"{LATEST_NATIVE}/result.json",
            "report_created_on": latest_native["reportCreatedOn"],
            "full_batch_success": latest_native["succeeded"], "full_batch_failed": latest_native["failed"],
            "process_errors": sum(item.get("severity") == "error" for item in native_result["diagnostics"]),
            "process_warnings": sum(item.get("severity") == "warning" for item in native_result["diagnostics"]),
            "classification": "Native13 full28=28P0F/0warnings/0test errors; TASK099 subset23=23P0F/0warnings/0test errors. 13 frame0 startup Smoke Condition failed entries and 1 EULA warning remain separate process diagnostics. Historical Native07 full24/subset19 and separate Native08 fresh Save1P remain bound to original reports.",
            "historical_voice_warning": "Native03 NPCCommittedSuccessUnbound missing fixed voice_state_task_finished.wav retained in original report; fixed recording remains UNPRODUCED, Native06 subset0warnings",
        },
        "historical_candidate2": {
            "candidate_id": "iteration-084-103-20261007-2", "version": "0.2.0-preview.20261007.1",
            "build": "BUILD_SUCCESS", "build_info": "docs/releases/iteration-084-103-rc/BUILD-INFO.json",
            "package_result": ".agent-local/qa/TASK-103/package-20261007-2/result.json",
            "package_evidence": "docs/qa/TASK-103/PACKAGE_BUILD_SUCCESS.json",
            "actual_embedded_version": "0.2.0-preview.20261007.1",
            "normal_input": "PARTIAL; unchanged original evidence and Continue erratum retained",
            "scope": "Historical uncommitted version.1 only; no candidate-4 final-pass credit",
        },
        "historical_candidate3": {
            "candidate_id": os3["candidate_id"], "version": os3["actual_embedded_version"],
            "package_evidence": "docs/qa/TASK-103/CANDIDATE3_PACKAGE_BUILD_SUCCESS.json",
            "os_source": OS3, "state": os3["state"], "launcher_method": os3["launcher_method"],
            "cpu_launcher": os3["cpu_launcher"], "vulkan_launcher": os3["vulkan_launcher"],
            "new_game": os3["new_game"], "journal": os3["journal"], "save": os3["save"],
            "continue": os3["continue"], "chinese_text": os3["chinese_text"],
            "cpu_model": os3["cpu_model"], "fresh_reset_status": os3["fresh_reset_status"],
            "normal_exit": os3["normal_exit"], "screenshots": os3["screenshots"],
            "scope": "CPU script launch and OS game input only; no Vulkan/Continue/IME/whole route/final-candidate credit",
        },
        "historical_candidate_source_freeze": "SECOND_ATTEMPT_FROZEN_UNCOMMITTED_WITH_CONFIG_MICROFIX",
        "historical_candidate_build": "BUILD_SUCCESS_SECOND_ATTEMPT",
        "historical_package_result": ".agent-local/qa/TASK-103/package-20261007-2/result.json",
        "historical_package_success_evidence": "docs/qa/TASK-103/PACKAGE_BUILD_SUCCESS.json",
        "actual_embedded_version": os5["actual_embedded_version"],
        "historical_actual_embedded_version": "0.2.0-preview.20261007.1",
        "historical_shipping_os_evidence": "docs/qa/TASK-103/OS_PARTIAL_NORMAL_INPUT.json",
        "historical_shipping_os_scope": "Candidate-2/version.1 Windows Run direct exe: title/new-game bedroom/J main01/F6 manual1→2/close/independent startup and retained save-list entries. Return Continue claim withdrawn; separate TASK-101 original-copy profile explicit mouse-click Continue restored camp-preparation and retained7 nodes. No model request or final-candidate claim.",
        "invalid_return_continue_claim": "docs/qa/TASK-103/OS_CONTINUE_ERRATUM.json",
        "historical_shipping_original_copy_compatibility_evidence": "docs/qa/TASK-101/OS_ORIGINAL_COPY_COMPATIBILITY.json",
        "historical_original_copy_compatibility_state": original_copy["state"],
        "historical_candidate4_original_copy_compatibility": {"source": "docs/qa/TASK-101/OS_CANDIDATE4_ORIGINAL_COPY_COMPATIBILITY.json", "safe_summary": original_copy4},
        "current_source_version": build_info["proposed_version"],
        "current_source_shipping": f"CANDIDATE5_BUILD_SUCCESS_PARTIAL_OS; externalZIP={zip_state}",
        "fixed_ai_contract_revision": os4["fixed_ai_contract_revision"],
        "current_candidate": {"sequence": 5, "proposed_version": build_info["proposed_version"],
                              "state": build_info["state"], "source_change_scope": build_info["source_change_scope"],
                              "reason": "Two existing guidance handover fixes Native RED→GREEN and real four-node API verified; full progression not completed. Exact Shipping5 success plus actual dual suppliedCMD script-launch/OS opening-save-Continue partial; no new model/IME/full-route credit."},
        "historical_candidate4": {"build": "docs/qa/TASK-103/CANDIDATE4_PACKAGE_BUILD_SUCCESS.json",
                                  "os_source": OS4, "os": os4, "zip": "PAUSED_NOT_CREATED"},
        "historical_candidate5_preparation": candidate5_prep,
        "candidate4_finalization": "PAUSED_BY_ROOT; package and OS partial evidence retained as historical candidate; final matrix awaits candidate5 actual evidence",
        "development_api_progression": {"source_report": NAVIGATION, "safe_summary": navigation,
                                         "state": navigation["status"], "owned_exit_verified": navigation["owned_editor"]["exit_verified"],
                                         "source_finding": "Both Source.4 guidance fixes independently Native RED→GREEN and actual four nodes verified. Nav12 grounded first segment ordinary complete query/controller path moved1976.913883cm; second valid=true/partial=true stopped before SimpleMove. No full escape/camp/newSave/OS; map/collision cause unproven.",
                                         "historical_projection_runs": {"source": NAV_HISTORY, "safe_summary": navigation_history},
                                         "historical_nav07": {"source_report": f"{NAVIGATION07}/results.json", "guidance_nodes": navigation07["guidance_nodes"], "owned_exit_verified": navigation07["owned_editor_exit_verified"]},
                                         "historical_nav06": {"source_report": f"{NAVIGATION06}/results.json", "guidance_nodes": navigation06["guidance_nodes"], "owned_exit_verified": navigation06["owned_editor_exit_verified"]},
                                         "historical_nav05": {"source_report": f"{NAVIGATION05}/results.json", "read_only_diagnostic": nav_read, "owned_exit_verified": navigation05["owned_editor_exit_verified"]},
                                         "historical_nav04": {"source_report": f"{NAVIGATION04}/results.json", "read_only_diagnostic": navigation04["nav_diagnostics"][0], "owned_exit_verified": navigation04["owned_editor_exit_verified"]},
                                         "shipping": "NOT_RUN", "normal_os_input": "NOT_RUN", "escape_camp_new_save": "NOT_REACHED",
                                         "prior_python_attempt": "HTTP400 security refusal; no driver or movement; historical body NOT_RETAINED"},
        "first_attempt_result": ".agent-local/qa/TASK-103/package-20261007-1/result.json",
        "first_attempt_cook_failure": "docs/qa/TASK-103/FIRST_COOK_FAIL.json",
        "historical_retry_state": "BUILD_SUCCESS",
        "config_microfix": "Config/DefaultGame.ini GameFeatureData AssetManager rule; root-owned, empty scan directories and AlwaysCook",
        "historical_source_freeze_evidence": [".agent-local/qa/TASK-103/package-20261007-2/tracked.patch",
                                   ".agent-local/qa/TASK-103/package-20261007-2/untracked-files.txt"],
        "remote_version_uniqueness": "UNKNOWN: gh 401, unauthenticated API 403 rate limit; cached web page insufficient",
        "rules": ["每条结果绑定自身原始index和dirty快照，不能迁移为最终候选PASS。",
                  "found=0表示本截点无原生证据，不代表游戏测试成功。",
                  "设计暂缓/工程缺项/真人环境分别登记，不泛化为全单需人工。"],
        "tasks": rows,
    })
    files = []
    for directory, children, names in os.walk(ROOT / "Resources"):
        children[:] = [x for x in children
                       if (Path(directory) / x).relative_to(ROOT).as_posix().lower() != "resources/ui/fonts"]
        for name in names:
            path = Path(directory) / name
            files.append({"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size})
    write_json("RESOURCE_INVENTORY.json", {
        "layer": "BOUNDED_FILENAME_AND_SIZE_AUDIT_NOT_ARCHIVE_SECRET_SCAN",
        "forbidden_not_traversed": ["Resources/UI/Fonts", "Runtime", "Saved", ".git", "Content"],
        "files": sorted(files, key=lambda x: x["path"]), "count": len(files),
        "total_bytes": sum(x["bytes"] for x in files),
        "suspicious_names": [x for x in files if any(word in Path(x["path"]).name.lower()
                             for word in (".env", "secret", "credential", "password", "token", "key", "pool.hws", "cache"))],
        "explicit_non_runtime_files_for_candidate_only_exclusion": [
            "Resources/UI/items-clean.prompt.txt", "Resources/UI/LAYOUT.md", "Resources/Audio/README.md"],
        "content_secret_scan": "Metadata inventory does not scan contents; candidate5 bounded-secret-scan.json is separate actual record",
        "archive_finalization": {"source": finalization_path, "state": zip_state, "result": finalization},
        "final_archive_secret_rescan": "See candidate5 finalizer bounded-secret-scan.json", "candidate_archive_inventory": "See final-file-manifest.json; separate from Resources metadata",
        "state": f"FROZEN_CANDIDATE5_METADATA; external archive state={zip_state}",
        "source_freeze_evidence": build_info["source_freeze_evidence"],
        "audio_contract": {"source": "Resources/Data/experience.json", "sound_event_count": len(events),
                           "unique_wav_count": len(wavs), "unique_wavs": wavs,
                           "environment_entry_count": len(audio["water"]),
                           "required_licence_files_at_install": 9,
                           "install_source": f"{PACKAGE}/document-install.json",
                           "water_geometry": "Root install verified exact loaded mesh/source-report association, 98 vertices/96 triangles; not licence proof",
                           "hearing_and_owner_audition": "NOT_RUN"},
        "hashes_computed": 0,
    })
    print(f"TASK-103 bounded audit: {len(rows)} task rows; {len(files)} Resources metadata rows; no UE invocation.")


if __name__ == "__main__":
    main()
