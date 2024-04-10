import { CallbackType } from '@lok/lok_enums';
import type { DocumentClient, DocumentWithViewMethods } from '@lok/shared';
import { Accessor, createRenderEffect, onCleanup } from 'solid-js';
import { IS_MAC } from './isMac';
import { PartialMouseEvent } from './vclMouse';

declare global {
  interface Navigator {
    /** navigator.platform is still the most supported and efficient way to check */
    readonly platform: string;
  }
  interface KeyboardEvent {
    readonly type: 'keydown' | 'keyup';
  }
}

// sourced from libreoffice-core/include/vcl/keycodes.hxx
export enum Modifiers {
  SHIFT = 0x1000,
  CTRL = 0x2000,
  ALT_OR_OPTION = 0x3000,
  META_OR_CMD = 0x8000,
}

// sourced indirectly from libreoffice-core/include/vcl/keycodes.hxx and directly from offapi/com/sun/star/awt/Key.idl
// mapped from https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_code_values
const VCL_KEY_CODES = {
  Backspace: 1283,
  Tab: 1282,
  Enter: 1280,
  Escape: 1281,
  Space: 1284,
  PageUp: 1030,
  PageDown: 1031,
  End: 1029,
  Home: 1028,
  ArrowLeft: 1026,
  ArrowUp: 1025,
  ArrowRight: 1027,
  ArrowDown: 1024,
  Insert: 1285,
  Delete: 1286,
  Digit0: 256,
  Digit1: 257,
  Digit2: 258,
  Digit3: 259,
  Digit4: 260,
  Digit5: 261,
  Digit6: 262,
  Digit7: 263,
  Digit8: 264,
  Digit9: 265,
  KeyA: 512,
  KeyB: 513,
  KeyC: 514,
  KeyD: 515,
  KeyE: 516,
  KeyF: 517,
  KeyG: 518,
  KeyH: 519,
  KeyI: 520,
  KeyJ: 521,
  KeyK: 522,
  KeyL: 523,
  KeyM: 524,
  KeyN: 525,
  KeyO: 526,
  KeyP: 527,
  KeyQ: 528,
  KeyR: 529,
  KeyS: 530,
  KeyT: 531,
  KeyU: 532,
  KeyV: 533,
  KeyW: 534,
  KeyX: 535,
  KeyY: 536,
  KeyZ: 537,
  Numpad0: 256,
  Numpad1: 257,
  Numpad2: 258,
  Numpad3: 259,
  Numpad4: 260,
  Numpad5: 261,
  Numpad6: 262,
  Numpad7: 263,
  Numpad8: 264,
  Numpad9: 265,
  NumpadMultiply: 1289,
  NumpadAdd: 1287,
  NumpadSubtract: 1288,
  NumpadDecimal: 1309,
  NumpadDivide: 1290,
  F1: 768,
  F2: 769,
  F3: 770,
  F4: 771,
  F5: 772,
  F6: 773,
  F7: 774,
  F8: 775,
  F9: 776,
  F10: 777,
  F11: 778,
  F12: 779,
  NumLock: 1313,
  ScrollLock: 1314,
  Minus: 1288,
  Semicolon: 1317,
  Equal: 1295,
  Comma: 1292,
} as const;

const SHOULD_HANDLE_KEYDOWN_ON_CODE = new Set<string>([
  'Tab',
  'Pause',
  'CapsLock',
  'Escape',
  'PageUp',
  'PageDown',
  'End',
  'Home',
  'ArrowLeft',
  'ArrowRight',
  'ArrowUp',
  'ArrowDown',
  'Insert',
  'F2',
]);

function vclKeyCode(code: keyof typeof VCL_KEY_CODES, keyCode: number) {
  return VCL_KEY_CODES[code] || keyCode;
}

export function pressKey(
  doc: DocumentClient,
  key: keyof typeof VCL_KEY_CODES,
  modifiers: number = 0
): void {
  const code = VCL_KEY_CODES[key] | modifiers;
  doc.postKeyEvent(KEY_DOWN, 0, code);
  doc.postKeyEvent(KEY_UP, 0, code);
}

export function eventModifiers(evt: PartialMouseEvent | KeyboardEvent): number {
  return (
    (evt.shiftKey ? Modifiers.SHIFT : 0) |
    (evt.ctrlKey ? Modifiers.CTRL : 0) |
    (evt.altKey ? Modifiers.ALT_OR_OPTION : 0) |
    (evt.metaKey ? Modifiers.META_OR_CMD : 0)
  );
}

export type VclKeyEvent = Parameters<DocumentWithViewMethods['postKeyEvent']>;

export type Shortcut = {
  /** The value for KeyboardEvent.key
  Reference: https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_key_values */
  key: string;
  modifiers: Array<'shift' | 'alt' | 'cmd' | 'ctrl'>;
};

function shortcutToMapKey(shortcut: Shortcut) {
  return [shortcut.key, ...shortcut.modifiers.sort()].join('+');
}

const KEY_DOWN = 0;
const KEY_UP = 1;
const ZERO_KEY = 48;

// TODO: add edit permissions
function userCanEdit(doc: Accessor<DocumentClient>) {
  return true;
}

export function createKeyHandler(
  doc: Accessor<DocumentClient>,
  textInputFocused: Accessor<boolean>
) {
  const ignoredShortcuts_ = new Set<string>();
  let lastCtrlAltLocation_: number | undefined = undefined;
  let isCursorVisible_: boolean = true;

  createRenderEffect(() => {
    const cb = (payload: string) => {
      isCursorVisible_ = payload === 'true';
    };
    const doc_ = doc();
    doc_.on(CallbackType.CURSOR_VISIBLE, cb);
    onCleanup(() => {
      doc_.off(CallbackType.CURSOR_VISIBLE, cb);
    });
  });

  return {
    /** Prevents sending the provided shortcuts to LOK */
    ignoreShortcuts(shortcuts: Shortcut[]): void {
      for (const shortcut of shortcuts) {
        ignoredShortcuts_.add(shortcutToMapKey(shortcut));
      }
    },
    handleKeyEvent(evt: KeyboardEvent): void {
      // Fixes AltGr => Alt + Ctrl on Windows
      if (evt.ctrlKey && evt.altKey) {
        // Ctrl+Alt/(AltGr)+ <char> doesn't give location information
        if (
          evt.type === 'keydown' &&
          evt.location === KeyboardEvent.DOM_KEY_LOCATION_RIGHT
        ) {
          lastCtrlAltLocation_ = evt.location;
          return;
        } else if (evt.location === KeyboardEvent.DOM_KEY_LOCATION_LEFT) {
          lastCtrlAltLocation_ = undefined;
        }

        if (
          lastCtrlAltLocation_ === KeyboardEvent.DOM_KEY_LOCATION_RIGHT &&
          evt.location === KeyboardEvent.DOM_KEY_LOCATION_STANDARD
        ) {
          return;
        }
      }
      let modifiers = eventModifiers(evt);

      // charCode/keyCode is deprecated, but it's used as a fallback if we somehow got here without a key code conversion
      const rawCharCode = (evt as KeyboardEvent & { charCode: number })
        .charCode;
      const rawKeyCode = (evt as KeyboardEvent & { keyCode: number }).keyCode;

      // handle "Option+<char>" on macOS
      if (
        IS_MAC &&
        (modifiers === Modifiers.ALT_OR_OPTION ||
          modifiers === Modifiers.ALT_OR_OPTION + Modifiers.SHIFT) &&
        rawKeyCode >= ZERO_KEY
      ) {
        modifiers -= Modifiers.ALT_OR_OPTION;
      }

      let vclCode = vclKeyCode(
        evt.code as keyof typeof VCL_KEY_CODES,
        rawKeyCode
      );
      vclCode |= modifiers;

      // simple key down
      if (
        modifiers &&
        evt.type === 'keydown' &&
        (!evt.shiftKey ||
          // don't override scrolling when out of focus
          (evt.code === 'Space' && !isCursorVisible_))
      ) {
        doc().postKeyEvent(KEY_DOWN, rawCharCode, vclCode);
        return evt.preventDefault();
      }

      if (!userCanEdit(doc)) {
        return evt.stopPropagation();
      }

      if (
        evt.type === 'keydown' &&
        SHOULD_HANDLE_KEYDOWN_ON_CODE.has(evt.code) &&
        rawCharCode === 0
      ) {
        doc().postKeyEvent(KEY_DOWN, rawCharCode, vclCode);
        if (
          !textInputFocused() ||
          (evt.code !== 'ArrowLeft' && evt.code !== 'ArrowRight')
        ) {
          evt.preventDefault();
        }
      } else if (
        evt.type === 'keyup' &&
        ((SHOULD_HANDLE_KEYDOWN_ON_CODE.has(evt.code) && rawCharCode === 0) ||
          modifiers ||
          evt.code === 'Enter')
      ) {
        doc().postKeyEvent(KEY_UP, rawCharCode, vclCode);
      }

      // prevent changing focus to another element on Tab
      if (evt.code === 'Tab') evt.preventDefault();
      evt.stopPropagation();
    },
  };
}
