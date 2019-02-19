var uvtest = require('bindings')('uvtest.node')
const EventEmitter = require('events').EventEmitter


exports.getEmitter = function() {
  const emitter = new EventEmitter()

  uvtest.callEmit(emitter.emit.bind(emitter))
  return emitter;
};
