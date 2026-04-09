# Released under the MIT License. See LICENSE for details.
#
"""Provides a QR code widget for web gamepad connection."""

from __future__ import annotations

import os
import sys

import bauiv1 as bui


def is_web() -> bool:
    """Return True if running on web/emscripten."""
    return sys.platform == 'emscripten'


def get_gamepad_url() -> str | None:
    """Return the web gamepad URL if available (web build only)."""
    if not is_web():
        return None
    try:
        path = os.path.expanduser('~/.ballisticakit/gamepad_url.txt')
        with open(path, 'r') as f:
            url = f.read().strip()
        return url if url else None
    except Exception:
        return None


class GamepadQRWidget:
    """Manages a QR code widget that polls for the gamepad URL."""

    def __init__(
        self,
        parent: bui.Widget,
        position: tuple[float, float],
        size: float = 130.0,
        transition_delay: float = 0.0,
    ):
        self._parent = parent
        self._position = position
        self._size = size
        self._tdelay = transition_delay
        self._img: bui.Widget | None = None
        self._label: bui.Widget | None = None
        self._loading_label: bui.Widget | None = None
        self._poll_timer: bui.AppTimer | None = None

        url = get_gamepad_url()
        if url:
            self._show_qr(url)
        else:
            self._show_loading()
            self._poll_timer = bui.AppTimer(
                0.5,
                bui.WeakCallStrict(self._poll_for_url),
                repeat=True,
            )

    def _show_loading(self) -> None:
        """Show animated loading placeholder."""
        x, y = self._position
        s = self._size

        # Pulsing dots as loading indicator.
        self._loading_label = bui.textwidget(
            parent=self._parent,
            position=(x, y + 5),
            size=(0, 0),
            scale=1.2,
            color=(1, 1, 1, 0.15),
            text='. . .',
            h_align='center',
            v_align='center',
            flatness=1.0,
            transition_delay=self._tdelay,
        )
        self._label = bui.textwidget(
            parent=self._parent,
            position=(x, y - s * 0.5 - 14),
            size=(0, 0),
            scale=0.5,
            color=(1, 1, 1, 0.2),
            text=bui.Lstr(value='Connecting gamepad service...'),
            h_align='center',
            v_align='center',
            flatness=1.0,
            maxwidth=s * 1.8,
            transition_delay=self._tdelay,
        )

    def _poll_for_url(self) -> None:
        url = get_gamepad_url()
        if url:
            self._poll_timer = None
            if self._loading_label:
                self._loading_label.delete()
                self._loading_label = None
            self._show_qr(url)

    def _show_qr(self, url: str) -> None:
        x, y = self._position
        s = self._size

        qr_tex = bui.get_qrcode_texture(url)
        self._img = bui.imagewidget(
            parent=self._parent,
            position=(x - s * 0.5, y - s * 0.5),
            size=(s, s),
            texture=qr_tex,
            has_alpha_channel=True,
            transition_delay=self._tdelay,
        )
        lbl_text = bui.Lstr(
            value='Scan to connect phone as gamepad'
        )
        if self._label:
            bui.textwidget(
                edit=self._label,
                text=lbl_text,
                color=(1, 1, 1, 0.35),
                scale=0.55,
            )
        else:
            self._label = bui.textwidget(
                parent=self._parent,
                position=(x, y - s * 0.5 - 14),
                size=(0, 0),
                scale=0.55,
                color=(1, 1, 1, 0.35),
                text=lbl_text,
                h_align='center',
                v_align='center',
                flatness=1.0,
                maxwidth=s * 1.8,
                transition_delay=self._tdelay,
            )

    def cleanup(self) -> None:
        """Remove all widgets created by this QR overlay."""
        self._poll_timer = None
        for w in (self._img, self._label, self._loading_label):
            if w is not None:
                try:
                    w.delete()
                except Exception:
                    pass
        self._img = None
        self._label = None
        self._loading_label = None
