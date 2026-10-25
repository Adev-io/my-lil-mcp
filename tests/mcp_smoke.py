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
                  'vcvrack_add_cable', 'vcvrack_delete_cable', 'vcvrack_search_library', 'vcvrack_get_rack_layout']
fails = 0


def check(ok, what, got=''):
    global fails
    print(('ok    ' if ok else 'FAIL  ') + what + ('' if got == '' else '  (%s)' % got))
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

if __name__ == '__main__':
    main()
