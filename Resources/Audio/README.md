# 固定对白人声

051已接通PCM16 WAV加载和字幕；当前54个逻辑cue、28个audio_group全部UNPRODUCED，没有提供人声文件。清单见`docs/planning/TASK-051/fixed-dialogue.csv`，运行表为`Resources/Data/experience.json`。

人工录制后按`Fixed/<audio_group>.wav`存放；使用标准RIFF WAV、PCM 16 bit、实际采样率与声道数。共享audio_group只制作一份同文本录音。交付需登记录制者、授权、文本版本及逐句试听结果，再更新voice_status；禁止把空音频或TTS登记为人工配音。

组件标签`Hearthward.Audio.voice`使主／人声音量相乘；字幕开关、字号、说话人和背景独立于音量。未制作时保留字幕和UNPRODUCED状态，不伪造声音通过证据。
