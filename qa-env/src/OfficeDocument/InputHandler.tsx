import { DocumentClient } from '@lok/shared';
import { Accessor, createEffect, onCleanup } from 'solid-js';
import { getOrCreateFocusedSignal } from './focus';
import { eventModifiers, pressKey } from './vclKeys';
import { CallbackType } from '@lok/lok_enums';

interface Props {
  doc: Accessor<DocumentClient>;
  pos: number[];
}

const FORWARD = 0;
const BACKWARD = 1;

// NOTE: this is a worrkaround because removetext events
// are not captured by the textarea if there is no input
// so we have to maintain a space at the start and end of the textarea
// at all times. This allows you to click anywhere and hold delete.
//
// Collabora uses a <img> tag with some special attributes to handle this
// but Teo found that regular spaces work just as well. Supposedly this is
// because Firefox collapses it to an empty string, but perhaps that was older
// versions.
const PRE_SPACE_CHAR = ' ';
const POST_SPACE_CHAR = ' ';
export const INITIAL_CONTENT = PRE_SPACE_CHAR + POST_SPACE_CHAR;

type TextAreaEvent = InputEvent & {
  currentTarget: HTMLTextAreaElement;
  target: HTMLTextAreaElement;
  inputType:
    | 'insertText'
    | 'insertLineBreak'
    | 'deleteContentBackward'
    | 'deleteContentForward'
    | 'insertFromPaste'
    | 'deleteByCut';
};

export function InputHandler(props: Props) {
  let input: HTMLTextAreaElement;
  const [focus] = getOrCreateFocusedSignal(props.doc);
  let composing = false;
  let lastContent: Array<number> = [];
  let ignoreInputLocks = 0;
  let lineBreakModifiers = 0;

  createEffect(() => {
    const handleUndoRedo = (payload: string) => {
      if (!payload) return;
      const { commandName } = JSON.parse(payload); // TODO: just check if this includes the correct "commandName": ".uno:Undo" JSON, etc.
      if (commandName === '.uno:Undo' || commandName === '.uno:Redo') reset();
    };
    const doc = props.doc();
    doc.on(CallbackType.UNO_COMMAND_RESULT, handleUndoRedo);
    onCleanup(() => {
      doc.off(CallbackType.UNO_COMMAND_RESULT, handleUndoRedo);
    });
  });

  createEffect(() => {
    if (focus()) {
      input.focus();
      if (!isSelectionValid(input) || isCaretAtPreSpace(input)) reset();
    } else {
      if (composing) abortComposition();
      composing = false;
    }
  });

  function abortComposition() {
    reset(document.activeElement !== input);
  }

  function handleInsertText() {
    const content = stringToUtf16CodePoints(inputValue(input));
    const len = matchingLength(content, lastContent);
    const insertedCodePoints = len > 0 ? content.slice(len) : content;
    if (insertedCodePoints.length > 0) {
      const lines = utf16CodePointsToString(insertedCodePoints).split(/[\n\r]/);
      for (let i = 0; i < lines.length; ++i) {
        if (i !== 0) {
          pressKey(props.doc(), 'Enter', lineBreakModifiers);
          reset();
        }
        if (lines[i].length > 0) {
          props.doc().postTextInput(0 /** default window */, lines[i]);
        }
      }
    }
    lastContent = content;
  }

  function handleDeleteContent(direction: number) {
    const content = stringToUtf16CodePoints(inputValue(input));
    const len = matchingLength(content, lastContent);
    let removeBefore = lastContent.length - len;
    let removeAfter = 0;

    // forward = delete, backwards = backspace
    if (lastContent.length > content.length && direction === FORWARD) {
      removeBefore--;
      removeAfter++;
    }

    if (removeBefore > 0 || removeAfter > 0) {
      props.doc().removeText(removeBefore, removeAfter);
    }

    if (removeAfter > 0 && direction === FORWARD) {
      props.doc().removeText(0, removeAfter);
      reset();
    } else if (isCaretAtPreSpace(input) && direction === BACKWARD) {
      props.doc().removeText(1, 0);
      reset();
    } else if (isCaretAtPostSpace(input) && direction === FORWARD) {
      props.doc().removeText(0, 1);
      reset();
    }

    lastContent = content;
  }

  function reset(noSelect: boolean = false) {
    ignoreInputLocks++;
    lastContent = [];
    input.focus();
    input.value = INITIAL_CONTENT; // pre and post space
    if (!noSelect) input.setSelectionRange(1, 1);
    ignoreInputLocks--;
  }

  function handleInput(evt: TextAreaEvent) {
    if (ignoreInputLocks > 0) {
      console.log('Ignoring synthetic input');
      return;
    }

    switch (evt.inputType) {
      case 'insertText':
        handleInsertText();
        break;
      case 'insertLineBreak':
        pressKey(props.doc(), 'Enter', lineBreakModifiers);
        reset();
        break;
      case 'deleteContentBackward':
        handleDeleteContent(BACKWARD);
        break;
      case 'deleteContentForward':
        handleDeleteContent(FORWARD);
        break;
      case 'insertFromPaste':
      case 'deleteByCut':
        reset();
        break;
    }
  }

  return (
    <textarea
      class="absolute top-0 left-0 pointer-events-none whitespace-pre caret-transparent opacity-0 resize-none"
      autocapitalize="off"
      autocomplete="off"
      spellcheck={false}
      wrap="off"
      rows={1}
      tabIndex={-1}
      style={{
        transform: `translate(${props.pos[0]}px, ${props.pos[1] + props.pos[3]}px)`,
        width: '1px',
        height: '1px',
      }}
      ref={(ref) => {
        input = ref;
        if (focus()) input.focus();
      }}
      onBeforeInput={() => {
        if (!getSelection()) reset();
      }}
      onInput={handleInput as any}
      onCompositionStart={() => {
        composing = true;
      }}
      onCompositionEnd={(evt) => {
        handleInput(evt as any);
        composing = false;
      }}
      onKeyDown={(evt) => {
        if (evt.key === 'Enter') {
          lineBreakModifiers = eventModifiers(evt);
        }
      }}
      onKeyUp={(evt) => {
        if (
          !composing &&
          (evt.key === 'ArrowLeft' ||
            evt.key === 'ArrowRight' ||
            evt.key === 'ArrowUp' ||
            evt.key === 'ArrowDown' ||
            evt.key === 'Home' ||
            evt.key === 'End' ||
            evt.key === 'PageUp' ||
            evt.key === 'PageDown' ||
            evt.key === 'Escape')
        ) {
          reset();
        }
      }}
      onCompositionUpdate={handleInput as any}
      value={INITIAL_CONTENT}
    />
  );
}

function inputValue(el: HTMLTextAreaElement) {
  const { value } = el;
  if (!value) return '';
  return value.substring(1, value.length - 1) || '';
}

function isSelectionValid(el: HTMLTextAreaElement) {
  return el.selectionStart > 0 && el.selectionEnd < el.value.length - 1; // between spaces
}

function isCaretAtPreSpace(el: HTMLTextAreaElement) {
  return getSelection()?.isCollapsed && el.selectionStart === 0; // at pre space
}

function isCaretAtPostSpace(el: HTMLTextAreaElement) {
  return getSelection()?.isCollapsed && el.selectionStart === el.value.length; // before post space
}

function stringToUtf16CodePoints(value: string): Array<number> {
  return Array.from(value).map((char) => char.codePointAt(0)) as Array<number>;
}

function utf16CodePointsToString(codePoints: Array<number>): string {
  return codePoints
    .map((codePoint) => String.fromCodePoint(codePoint))
    .join('');
}

function matchingLength(
  content: Array<number>,
  lastContent: Array<number>
): number {
  const sharedLength = Math.min(content.length, lastContent.length);
  let matchTo = 0;

  while (matchTo < sharedLength && content[matchTo] === lastContent[matchTo]) {
    matchTo++;
  }

  return matchTo;
}
