# 物理UI轨迹

工具：computer-use @oai/sky，Windows SendInput；脚本仅建立隔离夹具和观察状态，未调用Submit/Confirm。

采集：点击输入框→输入中文两份木材→发送→实际点击取消回复→重新发送→形成两份卡→Esc关闭→T重新打开，旧卡已消失→显式重发→点击加号变三份→点击确认→Esc关闭→观察真实取得3、携带归零、营地入库3。physical-results.json保留逐状态轨迹。

ui-physical-*.png是实际编辑器窗口截图；physical-candidate.png/physical-delivered.png由CaptureUI生成，仅作结构辅助，不与物理截图混用。

制作、维修和规则操作见physicalb-results.json和对应ui-physicalb-*.png。系统中文输入法组合窗口未单独验证；跨档陈旧面板由脚本PIE覆盖。

制作实操：输入两批箭矢→核对木材2、产量8→点击确认→真实入库8。维修：输入唯一自有石斧→核对耐久20、木材2绳索1→确认→真实耐久100，装备仍持有。规则：输入以后禁止采木材→核对规则卡→确认→仅此时新增typed_constraint ban:wood。观察器8项检查均通过。最后一次确认后观察器正常结束、runner关闭窗口，随后截图刷新报告无目标；结果与状态轨迹确认动作已完成，未重复点击。
