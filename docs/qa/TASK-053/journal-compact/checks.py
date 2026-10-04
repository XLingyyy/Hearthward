"""Record the repository's required current checks, including the known baseline failure."""
from pathlib import Path
import subprocess
import sys

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]
jobs = [
    ('repo-validation.txt', [sys.executable, '-X', 'utf8', 'scripts/validate_repo.py']),
    ('tool-tests.txt', [sys.executable, '-X', 'utf8', '-m', 'unittest', 'discover', '-s', 'scripts/tests', '-v']),
    ('baseline-scope.txt', [sys.executable, '-X', 'utf8', 'scripts/validate_repo.py', '--task', 'TASK-053', '--base', 'HEAD']),
    ('diff-check.txt', ['git', 'diff', '--check']),
]
for filename, command in jobs:
    result = subprocess.run(command, cwd=GAME, capture_output=True, text=True, encoding='utf-8', errors='replace')
    (QA / filename).write_text('Command: ' + repr(command) + '\nExit code: ' + str(result.returncode) + '\n' + result.stdout + result.stderr, encoding='utf-8')
    print(filename, result.returncode, flush=True)
