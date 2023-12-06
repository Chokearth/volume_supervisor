"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const os = require("os");
const linux_1 = require("./platforms/linux");
const osType = os.type();
let platformImplementation;
switch (osType) {
    case 'Linux':
        platformImplementation = linux_1.linux;
        break;
    default:
        throw new Error('Unsupported OS found: ' + osType);
}
console.log('OS type: ' + osType);
platformImplementation.getGlobalVolume().then((volume) => {
    console.log('Volume: ' + volume);
});
//# sourceMappingURL=index.js.map