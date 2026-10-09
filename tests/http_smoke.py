"""Check a running demo server on port 8080; this resets its simulation."""
import json
import argparse
import time
import urllib.request
import urllib.error

parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=8080)
args = parser.parse_args()
base = f'http://127.0.0.1:{args.port}'
def get(path):
    with urllib.request.urlopen(base + path, timeout=3) as response:
        return json.load(response)
def control(command):
    request = urllib.request.Request(base + '/api/control', data=command.encode(), headers={'Content-Type':'text/plain'})
    with urllib.request.urlopen(request, timeout=3) as response:
        assert response.status == 200
    time.sleep(.15)
city = get('/api/city')
assert len(city['nodes']) == 28 and len(city['lanes']) == 136
control('pause')
a = get('/api/state')
time.sleep(.25)
b = get('/api/state')
assert a['paused'] and a['stats']['time'] == b['stats']['time']
control('reset')
assert get('/api/state')['stats']['spawned'] == 0
control('spawn=10')
control('speed=5')
control('resume')
time.sleep(.5)
a = get('/api/state')
assert a['stats']['time'] > 0 and a['stats']['spawned'] > 0
assert a['spawnRate'] == 10 and a['timeScale'] == 5
assert a['routing'] == 'time'
assert all(len(lane) == 8 and lane[3] >= 0 and 0 <= lane[4] <= 1 for lane in a['lanes'])
assert len(a['laneCrashouts']) == len(city['lanes'])
assert sum(a['laneCrashouts']) == a['stats']['abandoned']
control('route=1,28')
route = get('/api/state')['route']
assert route['fastest']['reachable']
assert route['fastest']['seconds'] <= route['shortest']['seconds'] + 0.002
assert route['fastest']['nodes'][0] == 1 and route['fastest']['nodes'][-1] == 28
control('route=2147483647,28')
assert 'error' in get('/api/state')['route']
control('clear-route')
assert get('/api/state')['route'] is None
for request in [
    urllib.request.Request(base + '/api/control', data=b'speed=nan'),
    urllib.request.Request(base + '/api/control', data=b'route=999999999999999999,1'),
    urllib.request.Request(base + '/api/control', data=b'pause', headers={'Origin':'https://example.com'}),
    urllib.request.Request(base + '/api/state', headers={'Host':'example.com'}),
]:
    try:
        urllib.request.urlopen(request)
        raise AssertionError('Invalid command/origin accepted')
    except urllib.error.HTTPError as error:
        assert error.code in (400,403)
control('spawn=3')
control('speed=1')
control('reset')
print('PASS: city/state API, pause, reset, demand, speed, resume, route comparison, congestion, invalid control and local origin restrictions')
