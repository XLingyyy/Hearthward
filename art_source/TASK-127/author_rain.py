"""Original deterministic rain texture. No downloaded or model-generated recordings."""
from pathlib import Path
import wave,json
import numpy as np
root=Path(__file__).resolve().parents[2];out=root/'Resources/Audio/TASK-127';out.mkdir(parents=True,exist_ok=True)
rate=32000;n=rate*12;rng=np.random.default_rng(127)
noise=rng.normal(0,1,n)
# Circular filtering keeps the waveform and filtering history continuous at the loop seam.
spectrum=np.fft.rfft(noise);freq=np.fft.rfftfreq(n,1/rate)
curve=(freq/(freq+180))*np.exp(-freq/5200)/np.sqrt(1+freq/500)
rain=np.fft.irfft(spectrum*curve,n)
for _ in range(460):
    start=int(rng.integers(n));length=int(rng.integers(80,900));t=np.arange(length)/rate
    grain=rng.normal(0,.13,length)*np.exp(-t*180)*(1-np.exp(-t*1000))
    np.add.at(rain,(start+np.arange(length))%n,grain)
rain*=.32/max(abs(rain));pcm=(rain*32767).astype('<i2')
with wave.open(str(out/'light-rain-loop.wav'),'wb') as f:f.setnchannels(1);f.setsampwidth(2);f.setframerate(rate);f.writeframes(pcm.tobytes())
(out/'PROVENANCE.json').write_text(json.dumps({'source':'Original project procedural rain, authored 2026-10-14','generator':'art_source/TASK-127/author_rain.py','sample_rate':rate,'seconds':12,'channels':1,'peak_dbfs':float(20*np.log10(max(abs(rain)))),'human_audition':'PENDING'},indent=2),encoding='utf-8')
