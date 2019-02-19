declare module 'xrandr-evt' {
  import { EventEmitter }                           = require('events');

  function createEmiiter(): EventEmitter;
  let numberOfGreetings: number;
}
