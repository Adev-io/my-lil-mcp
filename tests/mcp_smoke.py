#!/usr/bin/env python3
"""mcp_smoke.py: drives a running Rack (rig or live) through POST /mcp and checks the answers.
Run: py -3 -I tests/mcp_smoke.py --port 2601   (the rig patch listens on 2601 so it never collides with a live Rack on 2600)
Adds a Fundamental VCO, edits it, undoes, measures, then deletes it: a patch is left as it was found.
Author: Ari Klopfer / A_Development. MIT"""
import argparse
import json
import sys
import time
import urllib.error
import urllib.request

EXPECTED_TOOLS = ['vcvrack_get_status', 'vcvrack_list_modules', 'vcvrack_get_module', 'vcvrack_add_module',
                  'vcvrack_delete_module', 'vcvrack_get_params', 'vcvrack_set_params', 'vcvrack_list_cables',
                  'vcvrack_add_cable', 'vcvrack_delete_cable', 'vcvrack_search_library', 'vcvrack_get_rack_layout',
                  'vcvrack_undo', 'vcvrack_redo', 'vcvrack_save_patch', 'vcvrack_bypass_module',
                  'vcvrack_move_module', 'vcvrack_get_module_state', 'vcvrack_set_module_state',
                  'vcvrack_find_unpatched', 'vcvrack_measure']
fails = 0


def check(ok, what, got=''):
    global fails
    print(('ok    ' if ok else 'FAIL  ') + what + ('' if got == '' else '  (%s)' % (got,)))
    if not ok:
        fails += 1


def status_of(base, path, headers):
    req = urllib.request.Request(base + path, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=5) as r:
            return r.status
    except urllib.error.HTTPError as e:
        return e.code


def rpc(base, method, params=None):
    body = {'jsonrpc': '2.0', 'id': 1, 'method': method}
    if params is not None:
        body['params'] = params
    req = urllib.request.Request(base + '/mcp', data=json.dumps(body).encode(), method='POST',
                                 headers={'Content-Type': 'application/json'})
    with urllib.request.urlopen(req, timeout=20) as r:
        return json.loads(r.read())


def call(base, tool, args=None):
    """One tools/call; the tool's JSON text decoded, or RuntimeError with the tool's message."""
    res = rpc(base, 'tools/call', {'name': tool, 'arguments': args or {}})['result']
    text = res['content'][0]['text']
    if res.get('isError'):
        raise RuntimeError('%s: %s' % (tool, text))
    return json.loads(text)


def module_ids(base):
    return {m['id'] for m in call(base, 'vcvrack_list_modules')}


def test_guard(base):
    check(status_of(base, '/status', {}) == 200, 'GET /status from a plain client')
    check(status_of(base, '/status', {'Host': 'evil.com'}) == 403, 'foreign Host refused')
    check(status_of(base, '/status', {'Origin': 'https://evil.com'}) == 403, 'foreign Origin refused')
    check(status_of(base, '/status', {'Origin': 'http://localhost:5173'}) == 200, 'loopback Origin allowed')
    check(status_of(base, '/modules', {}) == 404, 'REST routes are gone')


def test_tools_list(base):
    names = {t['name'] for t in rpc(base, 'tools/list')['result']['tools']}
    for n in EXPECTED_TOOLS:
        check(n in names, 'tools/list has ' + n)
    st = call(base, 'vcvrack_get_status')
    check(st['sampleRate'] > 0 and st['server'] == 'My lil MCP', 'get_status', st['sampleRate'])


def add_vco(base):
    lib = call(base, 'vcvrack_search_library', {'q': 'VCO'})
    fund = next((p for p in lib if p['slug'] == 'Fundamental'), None)
    check(fund is not None, 'Fundamental is installed')
    if fund is None:
        sys.exit(1)
    slug = 'VCO' if any(m['slug'] == 'VCO' for m in fund['modules']) else fund['modules'][0]['slug']
    pos = call(base, 'vcvrack_get_rack_layout')['suggested_positions'][0]
    vco = call(base, 'vcvrack_add_module', {'plugin_slug': 'Fundamental', 'module_slug': slug, 'x': pos['x'], 'y': pos['y']})
    check(vco['id'] in module_ids(base), 'add_module', vco['id'])
    return vco


def remove(base, vco):
    call(base, 'vcvrack_delete_module', {'id': vco['id']})
    time.sleep(0.5)   # deletion is queued to the UI thread
    check(vco['id'] not in module_ids(base), 'delete_module')


def main():
    urllib.request.install_opener(urllib.request.build_opener(urllib.request.ProxyHandler({})))
    ap = argparse.ArgumentParser()
    ap.add_argument('--port', type=int, default=2600)
    a = ap.parse_args()
    base = 'http://127.0.0.1:%d' % a.port
    test_guard(base)
    test_tools_list(base)
    vco = add_vco(base)
    try:
        for step in STEPS:
            step(base, vco)
    finally:
        remove(base, vco)
    print('FAILED (%d)' % fails if fails else 'all ok')
    sys.exit(1 if fails else 0)


STEPS = []   # (base, vco) -> None; later tasks append to this list


def test_undo_redo(base, vco):
    r = call(base, 'vcvrack_undo')
    check(r['action'] == 'MCP add module', 'first undo is our add', r['action'])
    check(vco['id'] not in module_ids(base), 'undo removes the module the agent added')
    call(base, 'vcvrack_redo')
    check(vco['id'] in module_ids(base), 'redo brings it back with the same id')
    params = call(base, 'vcvrack_get_params', {'moduleId': vco['id']})
    freq = next(p for p in params if p['name'].lower().startswith('freq'))
    r = call(base, 'vcvrack_set_params', {'moduleId': vco['id'], 'params': [{'id': freq['id'], 'value': freq['max'] + 100}]})
    check(r['applied'] == 1, 'set_params applied')
    now = next(p for p in call(base, 'vcvrack_get_params', {'moduleId': vco['id']}) if p['id'] == freq['id'])
    check(abs(now['value'] - freq['max']) < 1e-4, 'set_params clamps to the param range', now['value'])
    r = call(base, 'vcvrack_undo')
    check(r['action'] == 'MCP set params', 'undo names the action', r['action'])
    now = next(p for p in call(base, 'vcvrack_get_params', {'moduleId': vco['id']}) if p['id'] == freq['id'])
    check(abs(now['value'] - freq['value']) < 1e-4, 'undo restores the param', now['value'])
    # A feedback cable from the VCO's first output into its own first input: valid in Rack, needs no other module.
    c = call(base, 'vcvrack_add_cable', {'outputModuleId': vco['id'], 'outputId': 0, 'inputModuleId': vco['id'], 'inputId': 0})
    check(any(x['id'] == c['id'] for x in call(base, 'vcvrack_list_cables')), 'add_cable', c['id'])
    call(base, 'vcvrack_undo')
    check(not any(x['id'] == c['id'] for x in call(base, 'vcvrack_list_cables')), 'undo removes the cable')
    call(base, 'vcvrack_redo')
    call(base, 'vcvrack_delete_cable', {'id': c['id']})
    check(not any(x['id'] == c['id'] for x in call(base, 'vcvrack_list_cables')), 'delete_cable')
    call(base, 'vcvrack_undo')
    check(any(x['id'] == c['id'] for x in call(base, 'vcvrack_list_cables')), 'undo restores the deleted cable')
    call(base, 'vcvrack_delete_cable', {'id': c['id']})


STEPS.append(test_undo_redo)


def test_edit_tools(base, vco):
    import os, tempfile
    r = call(base, 'vcvrack_bypass_module', {'id': vco['id'], 'bypassed': True})
    check(call(base, 'vcvrack_get_module', {'module_id': vco['id']})['bypassed'] is True, 'bypass_module on')
    call(base, 'vcvrack_undo')
    check(call(base, 'vcvrack_get_module', {'module_id': vco['id']})['bypassed'] is False, 'undo un-bypasses')
    m = call(base, 'vcvrack_get_module', {'module_id': vco['id']})
    r = call(base, 'vcvrack_move_module', {'id': vco['id'], 'x': m['x'] + 300, 'y': m['y']})
    check(abs(r['x'] - (m['x'] + 300)) < 1 and abs(r['y'] - m['y']) < 1, 'move_module', (r['x'], r['y']))
    call(base, 'vcvrack_undo')
    check(abs(call(base, 'vcvrack_get_module', {'module_id': vco['id']})['x'] - m['x']) < 1, 'undo moves it back')
    state = call(base, 'vcvrack_get_module_state', {'id': vco['id']})
    check(state['model'] == vco['slug'] and 'params' in state, 'get_module_state is the module JSON')
    params = call(base, 'vcvrack_get_params', {'moduleId': vco['id']})
    freq = next(p for p in params if p['name'].lower().startswith('freq'))
    call(base, 'vcvrack_set_params', {'moduleId': vco['id'], 'params': [{'id': freq['id'], 'value': freq['min']}]})
    call(base, 'vcvrack_set_module_state', {'id': vco['id'], 'state': state})
    now = next(p for p in call(base, 'vcvrack_get_params', {'moduleId': vco['id']}) if p['id'] == freq['id'])
    check(abs(now['value'] - freq['value']) < 1e-4, 'set_module_state restores the params', now['value'])
    try:
        call(base, 'vcvrack_set_module_state', {'id': vco['id'], 'state': {'model': 'NotThisOne', 'params': []}})
        check(False, 'set_module_state refuses another model')
    except RuntimeError as e:
        check('model' in str(e), 'set_module_state refuses another model', str(e)[:60])
    un = call(base, 'vcvrack_find_unpatched')
    mine = next((u for u in un if u['id'] == vco['id']), None)
    check(mine is not None and len(mine['outputs']) >= 1 and len(mine['inputs']) >= 1, 'find_unpatched lists the VCO ports', (len(mine['inputs']), len(mine['outputs'])) if mine else 'VCO missing')
    path = os.path.join(tempfile.gettempdir(), 'my-lil-mcp-smoke.vcv')
    if os.path.exists(path):
        os.remove(path)
    r = call(base, 'vcvrack_save_patch', {'path': path})
    check(os.path.exists(path) and os.path.getsize(path) > 100 and r['bytes'] == os.path.getsize(path), 'save_patch writes the file', r['bytes'])
    os.remove(path)


STEPS.append(test_edit_tools)


def test_measure(base, vco):
    # Fundamental VCO sine: +/-5 V = -6.02 dBFS peak, -9.03 dBFS RMS against Rack's 10 V full scale.
    mcp = call(base, 'vcvrack_get_mcp_position')['id']
    m = call(base, 'vcvrack_measure', {'moduleId': vco['id'], 'outputId': 0, 'seconds': 1})
    check(m['samples'] > 20000, 'measure saw about a second of audio', m['samples'])
    check(abs(m['left']['peakDb'] + 6.02) < 0.3, 'VCO sine peaks at -6 dBFS', m['left']['peakDb'])
    check(abs(m['left']['rmsDb'] + 9.03) < 0.3, 'VCO sine RMS -9 dBFS', m['left']['rmsDb'])
    check(abs(m['left']['crestDb'] - 3.01) < 0.3, 'crest 3 dB', m['left']['crestDb'])
    check(m['left']['clippedSamples'] == 0 and abs(m['left']['dcOffset']) < 0.01, 'no clips, no DC')
    check(m['right']['rmsDb'] < -100, 'nothing patched into R reads as floor', m['right']['rmsDb'])
    check(abs(m['lufs'] + 9.7) < 0.6, 'one channel of -6 dBFS sine is about -9.7 LUFS', m['lufs'])
    check(not any(c['inputModuleId'] == mcp for c in call(base, 'vcvrack_list_cables')), 'temporary cable removed')
    try:
        call(base, 'vcvrack_measure', {'seconds': 0.2})
        check(False, 'measure without a source refuses')
    except RuntimeError as e:
        check('LISTEN' in str(e), 'measure without a source refuses', str(e)[:60])


STEPS.append(test_measure)

if __name__ == '__main__':
    main()
