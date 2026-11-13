"""Tiny client for the Unreal Editor MCP server (streamable HTTP).
Usage: python ue_mcp.py <tool> '<json args>'   e.g.  python ue_mcp.py list_toolsets '{}'
"""
import json, re, sys, urllib.request

URL = "http://127.0.0.1:8000/mcp"


def post(body, sid=None):
    headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
    if sid:
        headers["Mcp-Session-Id"] = sid
    req = urllib.request.Request(URL, data=json.dumps(body).encode(), headers=headers, method="POST")
    with urllib.request.urlopen(req, timeout=300) as r:
        text = r.read().decode("utf-8", "replace")
        sid = r.headers.get("Mcp-Session-Id") or sid
    if text.lstrip().startswith(("event:", "data:")):
        text = "".join(l[5:] for l in text.splitlines() if l.startswith("data:"))
    return (json.loads(text) if text.strip() else None), sid


def main():
    tool, args = sys.argv[1], json.loads(sys.argv[2] if len(sys.argv) > 2 else "{}")
    res, sid = post({"jsonrpc": "2.0", "id": 1, "method": "initialize",
                     "params": {"protocolVersion": "2025-03-26", "capabilities": {},
                                "clientInfo": {"name": "claude-code", "version": "1"}}})
    post({"jsonrpc": "2.0", "method": "notifications/initialized"}, sid)
    res, _ = post({"jsonrpc": "2.0", "id": 2, "method": "tools/call", "params": {"name": tool, "arguments": args}}, sid)
    if res and "result" in res:
        import base64, os
        for i, c in enumerate(res["result"].get("content", [])):
            if c.get("type") == "image":
                out = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"capture_{i}.png")
                open(out, "wb").write(base64.b64decode(c["data"]))
                print("IMAGE ->", out)
            else:
                t = c.get("text", str(c))
                m = re.search(r'"data"\s*:\s*"([A-Za-z0-9+/=]{200,})"', t)
                if m:
                    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), sys.argv[3] if len(sys.argv) > 3 else "capture.png")
                    open(out, "wb").write(base64.b64decode(m.group(1)))
                    print("IMAGE ->", out)
                else:
                    print(t)
        if res["result"].get("isError"):
            sys.exit(1)
    else:
        print(json.dumps(res, indent=1))


if __name__ == "__main__":
    main()
