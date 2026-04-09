#!/usr/bin/env python3
"""WebSocket-to-UDP proxy for BombSquad web clients.

Each WebSocket connection gets its own UDP socket to the game server.
Binary data passes through unchanged — the proxy is protocol-agnostic.

Usage:
    python3 ws_proxy.py [--ws-port 43250] [--game-host 127.0.0.1] [--game-port 43210]

Requires: pip install websockets
"""

import argparse
import asyncio
import socket
import struct
import websockets


async def handle_client(ws, game_host: str, game_port: int):
    """Handle one WebSocket client by bridging to a UDP socket."""
    # Create a UDP socket for this client.
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.setblocking(False)
    udp.bind(('', 0))  # ephemeral port
    local_port = udp.getsockname()[1]

    loop = asyncio.get_event_loop()
    addr = (game_host, game_port)
    print(f'[proxy] Client connected, UDP port {local_port} -> {addr}')

    # Task: forward UDP responses back to WebSocket.
    async def udp_to_ws():
        while True:
            try:
                data = await loop.sock_recv(udp, 10000)
                await ws.send(data)
            except (ConnectionError, websockets.exceptions.ConnectionClosed):
                break
            except OSError:
                break

    udp_task = asyncio.create_task(udp_to_ws())

    try:
        # Forward WebSocket messages to UDP.
        async for message in ws:
            if isinstance(message, bytes):
                await loop.sock_sendto(udp, message, addr)
    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        udp_task.cancel()
        udp.close()
        print(f'[proxy] Client disconnected (UDP port {local_port})')


async def main():
    parser = argparse.ArgumentParser(description='WS-to-UDP proxy for BombSquad')
    parser.add_argument('--ws-port', type=int, default=43250,
                        help='WebSocket listen port (default: 43250)')
    parser.add_argument('--game-host', default='127.0.0.1',
                        help='Game server host (default: 127.0.0.1)')
    parser.add_argument('--game-port', type=int, default=43210,
                        help='Game server UDP port (default: 43210)')
    args = parser.parse_args()

    print(f'[proxy] WebSocket listening on ws://0.0.0.0:{args.ws_port}')
    print(f'[proxy] Forwarding to UDP {args.game_host}:{args.game_port}')

    async with websockets.serve(
        lambda ws: handle_client(ws, args.game_host, args.game_port),
        '0.0.0.0',
        args.ws_port,
        compression=None,
        max_size=65536,
    ):
        await asyncio.Future()  # run forever


if __name__ == '__main__':
    asyncio.run(main())
