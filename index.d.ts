declare module 'xrandr-evt' {
  import { EventEmitter }                           from 'events';

  function createEmiiter(): EventEmitter;
  let numberOfGreetings: number;
}
