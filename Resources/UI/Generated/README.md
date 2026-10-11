# 日记选中图标

由 `scripts/ui/prepare_journal_highlights.py` 从现有 `Resources/UI/Atlases/journal.png` 和主题 UV 自动生成。沿用原图来源与授权；没有引入外部素材。算法保留原运行时的暖白 RGB、亮度覆盖率和透明度，正常图标仍使用原 atlas。

改动原图或 UV 后重新运行脚本，再以 `--check` 检查。生成物由现有 Resources NonUFS 清单打包；玩家无需 Python/Pillow，运行时只加载已生成 PNG。
