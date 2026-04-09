# Released under the MIT License. See LICENSE for details.
#
"""Provides the invite-link tab for web multiplayer."""

from __future__ import annotations

from typing import TYPE_CHECKING
from dataclasses import dataclass
from enum import Enum

import bauiv1 as bui

if TYPE_CHECKING:
    pass


def _js(code: str) -> None:
    """Execute JavaScript code."""
    import _baplus
    _baplus.web_js_run(code)


def _js_val(code: str) -> str:
    """Execute JavaScript and return string result."""
    import _baplus
    return _baplus.web_js_eval(code)


class InviteLinkGatherTab:
    """Tab for creating/joining games via invite link (WebRTC P2P)."""

    class SubTabType(Enum):
        HOST = 'host'
        JOIN = 'join'

    @dataclass
    class State:
        sub_tab_value: str = 'host'

    def __init__(self, window) -> None:
        import weakref
        self._window = weakref.ref(window)
        self._container: bui.Widget | None = None
        self._sub_tab = self.SubTabType.HOST
        self._host_status_text: bui.Widget | None = None
        self._invite_link_text: bui.Widget | None = None
        self._sub_tab_host_btn: bui.Widget | None = None
        self._sub_tab_join_btn: bui.Widget | None = None
        self._join_code_display: bui.Widget | None = None
        self._join_code: str = ''
        self._host_poll_timer: bui.AppTimer | None = None
        self._join_poll_timer: bui.AppTimer | None = None
        self._region_width = 0.0
        self._region_height = 0.0

    def on_activate(
        self,
        parent_widget: bui.Widget,
        tab_button: bui.Widget,
        region_width: float,
        region_height: float,
        region_left: float,
        region_bottom: float,
    ) -> bui.Widget:
        self._region_width = region_width
        self._region_height = region_height

        self._container = cnt = bui.containerwidget(
            parent=parent_widget,
            size=(region_width, region_height),
            position=(region_left, region_bottom),
            background=False,
            selection_loops_to_parent=True,
        )

        v = region_height - 55
        tab_w = 200
        tab_h = 45
        gap = 20
        total_w = tab_w * 2 + gap
        x_start = region_width * 0.5 - total_w * 0.5

        self._sub_tab_host_btn = bui.buttonwidget(
            parent=cnt,
            position=(x_start, v - tab_h * 0.5),
            size=(tab_w, tab_h),
            label=bui.Lstr(value='Host Game'),
            autoselect=True,
            on_activate_call=lambda: self._set_sub_tab(
                self.SubTabType.HOST
            ),
            color=(0.4, 0.6, 0.4),
            textcolor=(0.8, 1.0, 0.8),
        )
        self._sub_tab_join_btn = bui.buttonwidget(
            parent=cnt,
            position=(x_start + tab_w + gap, v - tab_h * 0.5),
            size=(tab_w, tab_h),
            label=bui.Lstr(value='Join Game'),
            autoselect=True,
            on_activate_call=lambda: self._set_sub_tab(
                self.SubTabType.JOIN
            ),
            color=(0.4, 0.4, 0.4),
            textcolor=(0.7, 0.7, 0.7),
        )

        self._set_sub_tab(self._sub_tab)

        # Check for auto-join from URL parameter.
        auto_code = _js_val('window._autoJoinCode || ""')
        if auto_code:
            _js('window._autoJoinCode = "";')
            self._sub_tab = self.SubTabType.JOIN
            self._set_sub_tab(self.SubTabType.JOIN)
            self._join_code = auto_code
            if self._join_code_display:
                bui.textwidget(
                    edit=self._join_code_display,
                    text=auto_code,
                    color=(0.3, 0.8, 1.0),
                )
            bui.apptimer(0.5, bui.WeakCallStrict(self._join_press))

        return cnt

    def _set_sub_tab(self, value: SubTabType) -> None:
        self._sub_tab = value
        if self._container is None:
            return

        if self._sub_tab_host_btn:
            bui.buttonwidget(
                edit=self._sub_tab_host_btn,
                color=(
                    (0.4, 0.6, 0.4)
                    if value is self.SubTabType.HOST
                    else (0.4, 0.4, 0.4)
                ),
                textcolor=(
                    (0.8, 1.0, 0.8)
                    if value is self.SubTabType.HOST
                    else (0.7, 0.7, 0.7)
                ),
            )
        if self._sub_tab_join_btn:
            bui.buttonwidget(
                edit=self._sub_tab_join_btn,
                color=(
                    (0.4, 0.6, 0.4)
                    if value is self.SubTabType.JOIN
                    else (0.4, 0.4, 0.4)
                ),
                textcolor=(
                    (0.8, 1.0, 0.8)
                    if value is self.SubTabType.JOIN
                    else (0.7, 0.7, 0.7)
                ),
            )

        children = self._container.get_children()
        for child in children:
            if (
                child != self._sub_tab_host_btn
                and child != self._sub_tab_join_btn
            ):
                child.delete()

        if value is self.SubTabType.HOST:
            self._build_host_tab()
        else:
            self._build_join_tab()

    def _build_host_tab(self) -> None:
        cnt = self._container
        assert cnt is not None
        w = self._region_width
        h = self._region_height
        cy = h * 0.45

        bui.textwidget(
            parent=cnt,
            position=(w * 0.5, cy + 80),
            size=(0, 0),
            text=bui.Lstr(
                value='Start a game and share the invite code\n'
                'with a friend so they can join.'
            ),
            h_align='center',
            v_align='center',
            scale=1.0,
            color=(0.7, 0.7, 0.7),
            maxwidth=w * 0.75,
            flatness=1.0,
        )

        self._host_status_text = bui.textwidget(
            parent=cnt,
            position=(w * 0.5, cy + 10),
            size=(0, 0),
            text=bui.Lstr(value='Press the button below to begin'),
            h_align='center',
            v_align='center',
            scale=1.0,
            color=(1, 1, 1, 0.4),
            maxwidth=w * 0.8,
            flatness=1.0,
        )

        self._invite_link_text = bui.textwidget(
            parent=cnt,
            position=(w * 0.5, cy - 50),
            size=(0, 0),
            text='',
            h_align='center',
            v_align='center',
            scale=0.9,
            color=(0.3, 0.8, 1.0),
            maxwidth=w * 0.85,
            flatness=1.0,
        )

        bui.buttonwidget(
            parent=cnt,
            position=(w * 0.5 - 130, cy - 140),
            size=(260, 65),
            label=bui.Lstr(value='Start Hosting'),
            on_activate_call=self._host_press,
            autoselect=True,
        )

    def _build_join_tab(self) -> None:
        cnt = self._container
        assert cnt is not None
        w = self._region_width
        h = self._region_height
        cy = h * 0.45

        bui.textwidget(
            parent=cnt,
            position=(w * 0.5, cy + 80),
            size=(0, 0),
            text=bui.Lstr(
                value='Enter the invite code shared\n'
                'by the host to join their game.'
            ),
            h_align='center',
            v_align='center',
            scale=1.0,
            color=(0.7, 0.7, 0.7),
            maxwidth=w * 0.75,
            flatness=1.0,
        )

        self._join_code_display = bui.textwidget(
            parent=cnt,
            position=(w * 0.5, cy + 10),
            size=(0, 0),
            text=bui.Lstr(value='No code entered'),
            h_align='center',
            v_align='center',
            scale=1.1,
            color=(1, 1, 1, 0.4),
            maxwidth=w * 0.7,
            flatness=1.0,
        )
        self._join_code = ''

        bui.buttonwidget(
            parent=cnt,
            position=(w * 0.5 - 120, cy - 55),
            size=(240, 55),
            label=bui.Lstr(value='Enter Code'),
            on_activate_call=self._enter_code_press,
            autoselect=True,
        )

        bui.buttonwidget(
            parent=cnt,
            position=(w * 0.5 - 100, cy - 130),
            size=(200, 60),
            label=bui.Lstr(value='Join'),
            on_activate_call=self._join_press,
            autoselect=True,
        )

    def _host_press(self) -> None:
        if self._host_status_text:
            bui.textwidget(
                edit=self._host_status_text,
                text=bui.Lstr(value='Starting...'),
                color=(1, 1, 0.3),
            )

        _js(
            'window._p2pHostCode = "";'
            'window.startP2PHost(function(id){'
            '  window._p2pHostCode = id;'
            '});'
        )
        self._host_poll_timer = bui.AppTimer(
            0.3,
            bui.WeakCallStrict(self._poll_host_code),
            repeat=True,
        )

    def _poll_host_code(self) -> None:
        code = _js_val('window._p2pHostCode || ""')
        if not code:
            return
        self._host_poll_timer = None

        if self._host_status_text:
            bui.textwidget(
                edit=self._host_status_text,
                text=bui.Lstr(value='Share this code with a friend:'),
                color=(0.6, 1, 0.6),
            )
        if self._invite_link_text:
            bui.textwidget(
                edit=self._invite_link_text,
                text=code,
                color=(0.3, 0.8, 1.0),
                scale=1.3,
            )
        # Show invite link in a prompt so user can copy it.
        invite_url = _js_val(
            'window.location.origin + window.location.pathname'
            ' + "?join=' + code + '"'
        )
        _js(
            'prompt("Share this link with a friend:", "'
            + invite_url + '");'
        )
        bui.screenmessage(
            bui.Lstr(value='Share the link with a friend!'),
            color=(0, 1, 0),
        )

    def _enter_code_press(self) -> None:
        """Open browser prompt to enter invite code."""
        result = _js_val('prompt("Enter invite code:", "") || ""')
        if result:
            self._join_code = result.strip()
            if self._join_code_display:
                bui.textwidget(
                    edit=self._join_code_display,
                    text=self._join_code,
                    color=(0.3, 0.8, 1.0),
                )

    def _join_press(self) -> None:
        code = getattr(self, '_join_code', '').strip()
        if not code:
            bui.screenmessage(
                bui.Lstr(value='Please enter an invite code'),
                color=(1, 0, 0),
            )
            return

        bui.screenmessage(
            bui.Lstr(value='Connecting...'),
            color=(1, 1, 0),
        )

        _js(
            'window._p2pJoinResult = "";'
            'window.joinP2P("' + code + '", function(ok){'
            '  window._p2pJoinResult = ok ? "ok" : "fail";'
            '});'
        )
        self._join_poll_timer = bui.AppTimer(
            0.3,
            bui.WeakCallStrict(self._poll_join_result),
            repeat=True,
        )

    def _poll_join_result(self) -> None:
        result = _js_val('window._p2pJoinResult || ""')
        if not result:
            return
        self._join_poll_timer = None
        _js('window._p2pJoinResult = "";')

        if result == 'ok':
            bui.screenmessage(
                bui.Lstr(value='Connected! Joining party...'),
                color=(0, 1, 0),
            )
            import bascenev1 as bs
            bs.connect_to_party('127.0.0.1', port=43210)
        else:
            bui.screenmessage(
                bui.Lstr(value='Failed to connect'),
                color=(1, 0, 0),
            )

    def on_deactivate(self) -> None:
        self._host_poll_timer = None
        self._join_poll_timer = None

    def save_state(self) -> None:
        try:
            bui.app.ui_v1.window_states[type(self)] = self.State(
                sub_tab_value=self._sub_tab.value
            )
        except Exception:
            pass

    def restore_state(self) -> None:
        try:
            state = bui.app.ui_v1.window_states.get(type(self))
            if isinstance(state, self.State):
                self._sub_tab = self.SubTabType(state.sub_tab_value)
        except Exception:
            pass
