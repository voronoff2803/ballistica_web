# Released under the MIT License. See LICENSE for details.
#
"""Networking related functionality."""

from __future__ import annotations

import sys
import ipaddress
from typing import TYPE_CHECKING

if sys.platform != 'emscripten':
    import socket
    import threading

if TYPE_CHECKING:
    pass


class NetworkSubsystem:
    """Network related app subsystem."""

    def __init__(self) -> None:
        import babase._env

        if sys.platform != 'emscripten':
            assert babase._env._g_net_warm_start_thread is not None
            babase._env._g_net_warm_start_thread.join()
            babase._env._g_net_warm_start_thread = None

            assert babase._env._g_net_warm_start_ssl_context is not None
            self.sslcontext = babase._env._g_net_warm_start_ssl_context
            babase._env._g_net_warm_start_ssl_context = None

            assert babase._env._g_net_warm_start_pool_manager is not None
            self.urllib3pool = babase._env._g_net_warm_start_pool_manager
            babase._env._g_net_warm_start_pool_manager = None

            self.zone_pings_lock = threading.Lock()
        else:
            self.sslcontext = None
            self.urllib3pool = None
            self.zone_pings_lock = None

        # Zone IDs mapped to average pings. This will remain empty
        # until enough pings have been run to be reasonably certain
        # that a nearby server has been pinged.
        self.zone_pings: dict[str, float] = {}

        # For debugging/progress.
        self.connectivity_state = ''
        self.transport_state = ''
        self.server_time_offset_hours: float | None = None

    def pre_interpreter_shutdown(self) -> None:
        """Called just before interpreter shuts down."""
        self.urllib3pool.clear()


def get_ip_address_type(addr: str) -> socket.AddressFamily:
    """Return an address-type given an address.

    Can be :attr:`socket.AF_INET` or :attr:`socket.AF_INET6`.
    """

    version = ipaddress.ip_address(addr).version
    if version == 4:
        return socket.AF_INET
    assert version == 6
    return socket.AF_INET6
