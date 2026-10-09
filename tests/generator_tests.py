"""Check generated topology and geometry independently of the simulator."""
import importlib.util
import math
from pathlib import Path

spec = importlib.util.spec_from_file_location('generator', Path(__file__).resolve().parents[1] / 'tools/generate_city.py')
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)
roads, layout = generator.generate(28, 42, 2, 18)
assert (roads, layout) == generator.generate(28, 42, 2, 18)
assert layout != generator.generate(28, 43, 2, 18)[1]
points = {int(row.split()[0]): tuple(map(float, row.split()[1:])) for row in layout if not row.startswith('#')}
edges, adjacency, flags = set(), {node: set() for node in points}, {}
for row in roads:
    if row.startswith('#'):
        continue
    a, b, light, length = row.split()
    a, b, light, length = int(a), int(b), int(light), float(length)
    assert a in points and b in points and a != b and length > 0
    assert flags.get((a, b), (light, length)) == (light, length)
    flags[a, b] = light, length
    edges.add(tuple(sorted((a, b))))
    adjacency[a].add(b)
seen, stack = set(), [next(iter(points))]
while stack:
    node = stack.pop()
    if node in seen:
        continue
    seen.add(node)
    stack.extend(adjacency[node] - seen)
assert len(seen) == len(points)
for a, b in edges:
    assert (a, b) in flags and (b, a) in flags
    assert math.dist(points[a], points[b]) > 55

def turn(a, b, c):
    return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
for a, b in edges:
    for c, d in edges:
        if len({a,b,c,d}) < 4:
            continue
        p,q,r,s = points[a],points[b],points[c],points[d]
        assert not (turn(p,q,r)*turn(p,q,s)<0 and turn(r,s,p)*turn(r,s,q)<0), 'Roads cross without a node'
assert len(edges) >= len(points), 'Network has no alternate paths'
print('PASS: seeded geometry, connected roads, consistent parallel lanes, alternate paths, no unmarked crossings')
