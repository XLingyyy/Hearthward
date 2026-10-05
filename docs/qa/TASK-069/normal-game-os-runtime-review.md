# TASK-069 正常启动与继续游戏实际 OS 路线

2026-10-05，Root 使用公共 UEClient 从 Bootstrap 正常 uncooked -game 启动。唯一开发存档池 c5eb9b97-b99d-41e9-8a1f-ce19708dc24e；未加载 Python 夹具、未定位角色、未授予物资、未发送控制台任务命令。菜单点击和按键由实际 OS 输入完成。官方 -game 模式依据：[Epic Running Unreal Engine](https://dev.epicgames.com/documentation/unreal-engine/running-unreal-engine)。

首次：新游戏→实际夜袭场景→E 领取遗物（进度0/3→1/3）→X弟弟跟随（2/3）→Tab背包→F6存档→点击保存当前进度，产生10-04 19:57手动节点及“快照已保存”。600秒 host 超时自动清理；首次正常退出未验证。

第二次：相同池冷重启→点击继续游戏→恢复夜袭屋内，任务2/3、弟弟跟随、等级2、经验50/135、回护符1、石制战斧1且耐久80/80、负重20.5/100；Escape返回HUD→Escape暂停→返回主菜单→鼠标确认→退出游戏→鼠标确认。正常游戏窗口消失，实际日志记录 UGameEngine::HandleExitCommand / RequestExitWithStatus(0,0) / LogExit Preparing to exit，随后Root完成标记，host退出0且stop_ok。

证据索引：[normal-game-os-evidence.json](normal-game-os-evidence.json)。原始JPEG：[继续HUD](normal-continue-os.jpg)、[继续背包](normal-continue-inventory-os.jpg)。仅对清晰可见状态进行比较，没有完整GUID/权威Save快照比较。实际运行参数-NoSound，音频未验证；未准备模型bundle、未提交AI委托；非Shipping、非Owner真人体验、非九页全流程验收。

实际正常OS发现且保存RED：主菜单页脚 RROW 越底；背包装饰标题增行、主手武器换行、页脚键帽与标签重叠，亮木地面使背包/属性文字低对比；暂停英文装饰与页脚越底；Save固定政策与动态安全文本同位重叠。截图 normal-title-os.jpg / normal-inventory-red-os.jpg / normal-pause-red-os.jpg / normal-save-red-os.jpg。按批准069可读性窗口局部施工，修复后须实际合成视口验证。
