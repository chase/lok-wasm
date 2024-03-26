import { splitProps, type JSX, onCleanup } from 'solid-js';

interface Props extends Omit<JSX.HTMLAttributes<HTMLDivElement>, 'onScroll'> {
  onScroll: (yPx: number) => void;
}

export function ScrollArea(props_: Props) {
  let internalRef: HTMLDivElement;
  const [local, others] = splitProps(props_, [
    'children',
    'onScroll',
    'class',
    'ref',
  ]);
  const handleScroll = (evt: Event) => {
    if (!evt.currentTarget) return;
    local.onScroll((evt.currentTarget as HTMLDivElement).scrollTop);
  };
  onCleanup(() => {
    internalRef.removeEventListener('scroll', handleScroll);
  });
  return (
    <div
      class={`${local.class} h-full w-full overflow-auto`}
      {...others}
      ref={(ref) => {
        (local.ref as Extract<(typeof local)['ref'], Function>)?.(ref);
        ref.addEventListener('scroll', handleScroll, { passive: true });
        internalRef = ref;
      }}
    >
      <div
        style={{
          /** this is a clever workaround so that the center never changes even with overflow */
          'padding-left': 'calc(100vw - 100%)',
        }}
      >
        {local.children}
      </div>
    </div>
  );
}
