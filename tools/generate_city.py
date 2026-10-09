#!/usr/bin/env python3
"""Generate a seeded irregular city, connected roads, and a matching drawing."""
import argparse
import heapq
import math
import random
from pathlib import Path


def generate(count, seed, lane_limit, typical_length):
    rng = random.Random(seed)
    # Rejection sampling with spatial buckets keeps intersections from bunching up.
    scale = math.sqrt(count / 28)
    rx, ry, separation = 420 * scale, 290 * scale, 56
    points, buckets = [], {}
    attempts = 0
    while len(points) < count:
        attempts += 1
        if attempts > count * 1000:
            raise ValueError('Could not place the requested nodes')
        angle, radius = rng.uniform(0, math.tau), math.sqrt(rng.random())
        x, y = rx * radius * math.cos(angle), ry * radius * math.sin(angle)
        key = (math.floor(x / separation), math.floor(y / separation))
        nearby = (p for dx in (-1, 0, 1) for dy in (-1, 0, 1)
                  for p in buckets.get((key[0] + dx, key[1] + dy), ()))
        if any(math.hypot(x - a, y - b) < separation for a, b in nearby):
            continue
        points.append((x, y))
        buckets.setdefault(key, []).append((x, y))

    def distance(a, b):
        return math.dist(points[a], points[b])

    # Euclidean minimum spanning tree guarantees connection and has no crossings.
    edges, degree = set(), [0] * count
    visited, best, parent = [False] * count, [math.inf] * count, [-1] * count
    best[0] = 0
    def add(a, b):
        edges.add(tuple(sorted((a, b))))
        degree[a] += 1
        degree[b] += 1
    for _ in range(count):
        a = min((i for i in range(count) if not visited[i]), key=best.__getitem__)
        visited[a] = True
        if parent[a] >= 0:
            add(a, parent[a])
        for b in range(count):
            if not visited[b]:
                d = distance(a, b)
                if d < best[b]:
                    best[b], parent[b] = d, a

    def cross(a, b, c):
        return ((b[0] - a[0]) * (c[1] - a[1]) -
                (b[1] - a[1]) * (c[0] - a[0]))
    def intersects(a, b, c, d):
        if len({a, b, c, d}) < 4:
            return False
        p, q, r, s = points[a], points[b], points[c], points[d]
        return cross(p, q, r) * cross(p, q, s) < 0 and cross(r, s, p) * cross(r, s, q) < 0

    # Add short local links to form varied blocks, preserving a planar street map.
    candidates = set()
    for a in range(count):
        for b in heapq.nsmallest(min(8, count - 1), (b for b in range(count) if b != a), key=lambda b: distance(a, b)):
            candidates.add(tuple(sorted((a, b))))
    ranked = sorted(candidates - edges, key=lambda e: distance(*e) * rng.uniform(.8, 1.25))
    target = round(count * 1.45)
    for a, b in ranked:
        if len(edges) >= target:
            break
        if degree[a] >= 4 or degree[b] >= 4:
            continue
        if any(intersects(a, b, c, d) for c, d in edges):
            continue
        add(a, b)

    lengths = sorted(distance(a, b) for a, b in edges)
    median = lengths[len(lengths) // 2]
    signals = {i for i, d in enumerate(degree) if d >= 3 and rng.random() < .8}
    roads = ['# start end traffic_light_at_end length', f'# Irregular city: {count} nodes; generation seed {seed}']
    layout = ['# node x y (drawing coordinates only)']
    for i, (x, y) in enumerate(points):
        layout.append(f'{i + 1} {x:.3f} {y:.3f}')
    for a, b in sorted(edges):
        length = max(.1, round(distance(a, b) / median * typical_length, 2))
        lanes = lane_limit if degree[a] >= 3 and degree[b] >= 3 else 1
        for start, end in ((a, b), (b, a)):
            for _ in range(lanes):
                roads.append(f'{start + 1} {end + 1} {int(end in signals)} {length:g}')
    return roads, layout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    number = parser.add_mutually_exclusive_group()
    number.add_argument('--nodes', type=int, help='Number of intersections (default 28)')
    number.add_argument('--size', type=int, help='Legacy count: creates size squared nodes, in an irregular layout')
    parser.add_argument('--seed', type=int, default=42, help='City generation seed')
    parser.add_argument('--lanes', type=int, default=2, help='Maximum lanes per direction')
    parser.add_argument('--length', type=float, default=18, help='Typical road length in simulation units')
    parser.add_argument('--output', type=Path, default=Path('generated-city'))
    args = parser.parse_args()
    if args.size is not None and args.size < 2:
        parser.error('--size must be at least 2')
    count = args.nodes if args.nodes is not None else args.size ** 2 if args.size is not None else 28
    if not 2 <= count <= 2500 or not 1 <= args.lanes <= 10 or not 0 < args.length < 1e6:
        parser.error('Use 2–2500 nodes, lanes 1–10, and a positive finite length below 1e6')
    roads, layout = generate(count, args.seed, args.lanes, args.length)
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / 'CITY.IN').write_text('\n'.join(roads) + '\n')
    (args.output / 'CITY.LAYOUT').write_text('\n'.join(layout) + '\n')
    print(f'Wrote {count} nodes and {len(roads) - 2} directed lanes to {args.output}')


if __name__ == '__main__':
    main()
