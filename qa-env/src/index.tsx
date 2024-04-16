/* @refresh reload */
import { render } from 'solid-js/web'
import { preload, setIsMacOSForConfig } from '@lok';

import './index.css'
import App from './App'
import { IS_MAC } from './OfficeDocument/isMac';

preload();
if (IS_MAC) {
  setIsMacOSForConfig();
}
const root = document.getElementById('root')

render(() => <App />, root!)
