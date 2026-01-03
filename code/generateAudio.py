from collections import defaultdict
from mido import MidiFile

mid = MidiFile('tetris.mid')

# Get tempo and ticks per beat
tempo = 500000  # default microseconds per beat
ticks_per_beat = mid.ticks_per_beat
# Use float for division to avoid integer truncation
ticks_per_16th = ticks_per_beat / 4.0

# Collect note intervals (start_tick, end_tick, note)
intervals = []
note_on_stacks = defaultdict(list)  # key: (note, channel) -> list of start times
global_last_time = 0

for track in mid.tracks:
  current_time = 0
  for msg in track:
    current_time += msg.time
    if msg.type == 'set_tempo':
      tempo = msg.tempo
    if msg.type == 'note_on' and getattr(msg, 'velocity', 0) > 0:
      key = (msg.note, getattr(msg, 'channel', 0))
      note_on_stacks[key].append(current_time)
    elif msg.type == 'note_off' or (msg.type == 'note_on' and getattr(msg, 'velocity', 0) == 0):
      key = (msg.note, getattr(msg, 'channel', 0))
      if note_on_stacks[key]:
        start = note_on_stacks[key].pop()
        intervals.append((start, current_time, msg.note))
    if current_time > global_last_time:
      global_last_time = current_time

# Close any notes that never received a note_off
for key, starts in note_on_stacks.items():
  for start in starts:
    intervals.append((start, global_last_time, key[0]))

if not intervals:
  print([])
  raise SystemExit(0)

# Determine the number of 16th-note slots needed
max_end_tick = max(end for (_, end, _) in intervals)
max_index = int(round(max_end_tick / ticks_per_16th))

# Initialize result list with 0 (silence)
result = [0] * (max_index + 1)

def midi_to_freq(note):
  return 440.0 * (2 ** ((note - 69) / 12.0))

def freq_to_t2reload(freq):
  return round(65536 - 1382400/freq)

# Fill in result: onset -> frequency, sustain -> 1, silence -> 0
for start_tick, end_tick, note in sorted(intervals, key=lambda x: x[0]):
  start_idx = int(round(start_tick / ticks_per_16th))
  end_idx = int(round(end_tick / ticks_per_16th))
  if end_idx <= start_idx:
    end_idx = start_idx + 1
  freq = midi_to_freq(note)/4
  # Put frequency at onset if slot is empty (preserve earlier notes)
  if result[start_idx] == 0:
    result[start_idx] = freq_to_t2reload(freq)
  # Mark sustained slots with 1
  for i in range(start_idx + 1, end_idx):
    if result[i] == 0:
      result[i] = 1

print(result)