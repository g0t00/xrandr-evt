const xrandrEvt = require('./index');
const emitter = xrandrEvt.getEmitter();
emitter.on('change', () => {
  console.log('change');
});
