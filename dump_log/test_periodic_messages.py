import re
from collections import defaultdict
import statistics

line_re = re.compile(r"\(([\d\.]+)\)\s+\w+\s+([0-9A-Fa-f]+)#")
timestamps_by_id = defaultdict(list)
unmatched_lines = 0

with open("candump-2025-06-06_095231.log", "r") as f:
    for line in f:
        match = line_re.search(line)
        if match:
            timestamp = float(match.group(1))
            can_id = match.group(2)
            timestamps_by_id[can_id].append(timestamp)
        else:
            unmatched_lines += 1

for can_id, timestamps in timestamps_by_id.items():
    if len(timestamps) < 2:
        continue
    periods = [t2 - t1 for t1, t2 in zip(timestamps, timestamps[1:])]
    avg_period = sum(periods) / len(periods)
    std_period = statistics.stdev(periods)
    max_period = max(periods)
    min_period = min(periods)
    print(f"ID {can_id}: Periodicidad media = {avg_period:.6f} s, Desviación típica = {std_period:.6f} s, Máxima = {max_period:.6f} s, Mínima = {min_period:.6f} s, Mensajes = {len(timestamps)}")

if unmatched_lines:
    print(f"\nLíneas no reconocidas: {unmatched_lines}")
