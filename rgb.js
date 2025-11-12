
const JsonIOBridge = require('../JsonIOBridge');

const rgb = new JsonIOBridge(
  'rgb',
  './rgb/rgb_config_nodejs.json',
  './rgb/rgb_config_cpp.json'
);

//=============================================================

// rgb.write({ command: 'idle'});
// rgb.write({ command: 'indicate', red: 50, green: 100, blue: 0 });


//=============================================================

// const util = require('util');

//--------------------------------------------------------------

// rgb.read((data) => {
//   console.clear();
//   console.log(util.inspect(data, { colors: true, depth: null }));
// });

//--------------------------------------------------------------

// rgb.watch((data) => {
//   console.clear();
//   console.log(util.inspect(data, { colors: true, depth: null }));
// });

//=============================================================

module.exports = {rgb};

