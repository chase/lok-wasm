/* @refresh reload */
import { render } from 'solid-js/web'
import { preload, setIsMacOSForConfig } from '@lok';

import './index.css'
import App from './App'
import { IS_MAC } from './OfficeDocument/isMac';

if (IS_MAC) {
  await setIsMacOSForConfig();
}
preload();

const root = document.getElementById('root')

render(() => <App />, root!)
