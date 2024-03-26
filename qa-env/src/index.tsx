/* @refresh reload */
import { render } from 'solid-js/web'
import { preload } from '@lok';

import './index.css'
import App from './App'

preload();
const root = document.getElementById('root')

render(() => <App />, root!)
